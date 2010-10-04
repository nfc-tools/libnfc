/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2010, Roel Verdult
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
 * @file nfc-emulate-ndef.c
 * @brief Emulate a NFC Forum Tag Type 4 with a NDEF message
 */

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
// FIXME doesn't use a pn53x specific function, use libnfc's API
#include "chips/pn53x.h"

#define MAX_FRAME_LEN 264

static byte_t abtRx[MAX_FRAME_LEN];
static size_t szRxLen;
static nfc_device_t *pnd;
static bool quiet_output = false;

#define SYMBOL_PARAM_fISO14443_4_PICC   0x20

bool
transmit_bytes (const byte_t * pbtTx, const size_t szTxLen)
{
  // Show transmitted command
  if (!quiet_output) {
    printf ("Sent data: ");
    print_hex (pbtTx, szTxLen);
  }

  // Transmit the command bytes
  if (!nfc_target_send_bytes(pnd, pbtTx, szTxLen)) {
    nfc_perror (pnd, "nfc_target_send_bytes");
    return false;
  }

  if (!nfc_target_receive_bytes(pnd,abtRx,&szRxLen)) {
    nfc_perror (pnd, "nfc_target_receive_bytes");
    return false;
  }

  // Show received answer
  if (!quiet_output) {
    printf ("Received data: ");
    print_hex (abtRx, szRxLen);
  }
  // Succesful transfer
  return true;
}

int
main (int argc, char *argv[])
{
  // Try to open the NFC reader
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    ERR("Unable to connect to NFC device");
    return EXIT_FAILURE;
  }

  printf ("Connected to NFC device: %s\n", pnd->acName);
  printf ("Emulating NDEF tag now, please touch it with a second NFC device\n");

  nfc_target_t nt = {
    .ntt = NTT_MIFARE,
    .nti.nai.abtAtqa = { 0x00, 0x04 },
    .nti.nai.abtUid = { 0x08, 0x00, 0xb0, 0x0b },
    .nti.nai.btSak = 0x20,
    .nti.nai.szUidLen = 4,
    .nti.nai.szAtsLen = 0,
  };

  // FIXME doesn't use a pn53x specific function, use libnfc's API
  pn53x_set_parameters(pnd,SYMBOL_PARAM_fISO14443_4_PICC);

  if (!nfc_target_init (pnd, NTM_ISO14443_4_PICC, nt, abtRx, &szRxLen)) {
    nfc_perror (pnd, "nfc_target_init");
    ERR("Could not come out of auto-emulation, no command was received");
    return EXIT_FAILURE;
  }

  if (!quiet_output) {
    printf ("Received data: ");
    print_hex (abtRx, szRxLen);
  }

//Receiving data: e0  40
//= RATS, FSD=48
//Actually PN532 already sent back the ATS so nothing to send now
  transmit_bytes((const byte_t*)"\x0a\x00\x6a\x87",4);
//Receiving data: 0b  00      00  a4  04  00  06  e1  03  e1  03  e1  03
//= App Select by name "e103e103e103"
  transmit_bytes((const byte_t*)"\x0b\x00\x6a\x87",4);
//Receiving data: 0a  00      00  a4  04  00  07  d2  76  00  00  85  01  00
//= App Select by name "D2760000850100"
  transmit_bytes((const byte_t*)"\x0a\x00\x90\x00",4);
//Receiving data: 0b  00      00  a4  00  00  02  e1  03
//= Select CC
  transmit_bytes((const byte_t*)"\x0b\x00\x90\x00",4);
//Receiving data: 0a  00      00  b0  00  00  0f
//= ReadBinary CC
//We send CC + OK
  transmit_bytes((const byte_t*)"\x0a\x00\x00\x0f\x10\x00\x3b\x00\x34\x04\x06\xe1\x04\x0e\xe0\x00\x00\x90\x00",19);
//Receiving data: 0b  00      00  a4  00  00  02  e1  04
//= Select NDEF
  transmit_bytes((const byte_t*)"\x0b\x00\x90\x00",4);
//Receiving data: 0a  00      00  b0  00  00  02
//=  Read first 2 NDEF bytes
//Sent NDEF Length=0x21
  transmit_bytes((const byte_t*)"\x0a\x00\x00\x21\x90\x00",6);
//Receiving data: 0b  00      00  b0  00  02  21
  transmit_bytes((const byte_t*)"\x0b\x00\xd1\x02\x1c\x53\x70\x91\x01\x09\x54\x02\x65\x6e\x4c\x69\x62\x6e\x66\x63\x51\x01\x0b\x55\x03\x6c\x69\x62\x6e\x66\x63\x2e\x6f\x72\x67\x90\x00",37);
//Receiving data: ca  00
//= S(DESELECT)
// We don't have to reply ourselves because when using
// ISO/IEC14443-4 PICC mode, S(DESELECT) response is automatic

  nfc_disconnect(pnd);
  exit (EXIT_SUCCESS);
}

