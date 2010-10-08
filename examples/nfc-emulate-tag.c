/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2010, Romuald Conty
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file nfc-emulate-tag.c
 * @brief Emulate a simple tag
 */

// Note that depending on the device (initiator) you'll use against, this
// emulator it might work or not. Some readers are very strict on responses
// timings, e.g. a Nokia NFC and will drop communication too soon for us.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nfc/nfc.h>

#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

static byte_t abtRx[MAX_FRAME_LEN];
static size_t szRxLen;
static nfc_device_t *pnd;
static bool quiet_output = false;
static bool init_mfc_auth = false;

bool 
target_io( const nfc_target_t nt, const byte_t * pbtInput, const size_t szInput, byte_t * pbtOutput, size_t *pszOutput )
{
  bool loop = true;
  *pszOutput = 0;

  // Show transmitted command
  if (!quiet_output) {
    printf ("    In: ");
    print_hex (pbtInput, szInput);
  }
  if(szInput) {
    switch(pbtInput[0]) {
      case 0x60: // Mifare authA
      case 0x61: // Mifare authB
        // Let's give back a very random nonce...
        *pszOutput = 2;
        pbtOutput[0] = 0x12;
        pbtOutput[1] = 0x34;
        // Next commands will be without CRC
        init_mfc_auth = true;
        break;
      case 0xe0: // RATS
        // Send ATS
        *pszOutput = nt.nti.nai.szAtsLen + 1;
        pbtOutput[0] = nt.nti.nai.szAtsLen + 1; // ISO14443-4 says that ATS contains ATS_Lenght as first byte
        if(nt.nti.nai.szAtsLen) {
          memcpy(pbtOutput+1, nt.nti.nai.abtAts, nt.nti.nai.szAtsLen);
        }
        break;
      case 0xc2: // S-block DESELECT
        if (!quiet_output) {
          printf("Target released me. Bye!\n");
        }
        loop = false;
        break;
      default: // Unknown
        if (!quiet_output) {
          printf("Unknown frame, emulated target abort.\n");
        }
        loop = false;
    }
  }
  // Show transmitted command
  if ((!quiet_output) && *pszOutput) {
    printf ("    Out: ");
    print_hex (pbtOutput, *pszOutput);
  }
  return loop;
}

bool
nfc_target_emulate_tag(nfc_device_t* pnd, const nfc_target_t nt)
{
  size_t szTx;
  byte_t abtTx[MAX_FRAME_LEN];
  bool loop = true;

  if (!nfc_target_init (pnd, NTM_PASSIVE, nt, abtRx, &szRxLen)) {
    nfc_perror (pnd, "nfc_target_init");
    return false;
  }

  while ( loop ) {
    loop = target_io( nt, abtRx, szRxLen, abtTx, &szTx );
    if (szTx) {
      if (!nfc_target_send_bytes(pnd, abtTx, szTx)) {
        nfc_perror (pnd, "nfc_target_send_bytes");
        return false;
      }
    }
    if ( loop ) {
      if ( init_mfc_auth ) {
        nfc_configure (pnd, NDO_HANDLE_CRC, false);
        init_mfc_auth = false;
      }
      if (!nfc_target_receive_bytes(pnd, abtRx, &szRxLen)) {
        nfc_perror (pnd, "nfc_target_receive_bytes");
        return false;
      }
    }
  }
  return true;
}

int
main (int argc, char *argv[])
{
  const char *acLibnfcVersion;

  // Try to open the NFC reader
  pnd = nfc_connect (NULL);

  // Display libnfc version
  acLibnfcVersion = nfc_version ();
  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  if (pnd == NULL) {
    ERR("Unable to connect to NFC device");
    return EXIT_FAILURE;
  }

  printf ("Connected to NFC device: %s\n", pnd->acName);
  
  // Example of a Mifare Classic Mini
  // Note that crypto1 is not implemented in this example
  nfc_target_t nt = {
    .ntt = NTT_MIFARE,
    .nti.nai.abtAtqa = { 0x00, 0x04 },
    .nti.nai.abtUid = { 0x08, 0xab, 0xcd, 0xef },
    .nti.nai.btSak = 0x09,
    .nti.nai.szUidLen = 4,
    .nti.nai.szAtsLen = 0,
  };
/*
  // Example of a ISO14443-4 (DESfire)
  nfc_target_t nt = {
    .ntt = NTT_MIFARE,
    .nti.nai.abtAtqa = { 0x03, 0x44 },
    .nti.nai.abtUid = { 0x08, 0xab, 0xcd, 0xef },
    .nti.nai.btSak = 0x20,
    .nti.nai.szUidLen = 4,
    .nti.nai.abtAts = { 0x75, 0x77, 0x81, 0x02, 0x80 },
    .nti.nai.szAtsLen = 5,
  };
*/

  printf ("%s will emulate this ISO14443-A tag:\n", argv[0]);
  print_nfc_iso14443a_info( nt.nti.nai );

  printf ("NFC device (configured as target) is now emulating the tag, please touch it with a second NFC device (initiator)\n");
  if (!nfc_target_emulate_tag (pnd, nt)) {
    nfc_perror (pnd, "nfc_target_emulate_tag");
    return EXIT_FAILURE;
  }

  nfc_disconnect(pnd);
  exit (EXIT_SUCCESS);
}

