/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-emulate-tag.c
 * @brief Emulates a simple tag
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
#include <signal.h>

#include <nfc/nfc.h>

#include "utils/nfc-utils.h"

#define MAX_FRAME_LEN (264)
#define SAK_ISO14443_4_COMPLIANT 0x20

static uint8_t abtRx[MAX_FRAME_LEN];
static int szRx;
static nfc_context *context;
static nfc_device *pnd;
static bool quiet_output = false;
static bool init_mfc_auth = false;

static void
intr_hdlr(int sig)
{
  (void) sig;
  printf("\nQuitting...\n");
  if (pnd != NULL) {
    nfc_abort_command(pnd);
  }
  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_FAILURE);
}

static bool
target_io(nfc_target *pnt, const uint8_t *pbtInput, const size_t szInput, uint8_t *pbtOutput, size_t *pszOutput)
{
  bool loop = true;
  *pszOutput = 0;

  // Show transmitted command
  if (!quiet_output) {
    printf("    In: ");
    print_hex(pbtInput, szInput);
  }
  if (szInput) {
    switch (pbtInput[0]) {
      case 0x30: // Mifare read
        // block address is in pbtInput[1]
        *pszOutput = 15;
        strcpy((char *)pbtOutput, "You read block ");
        pbtOutput[15] = pbtInput[1];
        break;
      case 0x50: // HLTA (ISO14443-3)
        if (!quiet_output) {
          printf("Initiator HLTA me. Bye!\n");
        }
        loop = false;
        break;
      case 0x60: // Mifare authA
      case 0x61: // Mifare authB
        // Let's give back a very random nonce...
        *pszOutput = 2;
        pbtOutput[0] = 0x12;
        pbtOutput[1] = 0x34;
        // Next commands will be without CRC
        init_mfc_auth = true;
        break;
      case 0xe0: // RATS (ISO14443-4)
        // Send ATS
        *pszOutput = pnt->nti.nai.szAtsLen + 1;
        pbtOutput[0] = pnt->nti.nai.szAtsLen + 1; // ISO14443-4 says that ATS contains ATS_Length as first byte
        if (pnt->nti.nai.szAtsLen) {
          memcpy(pbtOutput + 1, pnt->nti.nai.abtAts, pnt->nti.nai.szAtsLen);
        }
        break;
      case 0xc2: // S-block DESELECT
        if (!quiet_output) {
          printf("Initiator DESELECT me. Bye!\n");
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
    printf("    Out: ");
    print_hex(pbtOutput, *pszOutput);
  }
  return loop;
}

static bool
nfc_target_emulate_tag(nfc_device *dev, nfc_target *pnt)
{
  size_t szTx;
  uint8_t abtTx[MAX_FRAME_LEN];
  bool loop = true;

  if ((szRx = nfc_target_init(dev, pnt, abtRx, sizeof(abtRx), 0)) < 0) {
    nfc_perror(dev, "nfc_target_init");
    return false;
  }

  while (loop) {
    loop = target_io(pnt, abtRx, (size_t) szRx, abtTx, &szTx);
    if (szTx) {
      if (nfc_target_send_bytes(dev, abtTx, szTx, 0) < 0) {
        nfc_perror(dev, "nfc_target_send_bytes");
        return false;
      }
    }
    if (loop) {
      if (init_mfc_auth) {
        nfc_device_set_property_bool(dev, NP_HANDLE_CRC, false);
        init_mfc_auth = false;
      }
      if ((szRx = nfc_target_receive_bytes(dev, abtRx, sizeof(abtRx), 0)) < 0) {
        nfc_perror(dev, "nfc_target_receive_bytes");
        return false;
      }
    }
  }
  return true;
}

int
main(int argc, char *argv[])
{
  (void) argc;
  const char *acLibnfcVersion;

#ifdef WIN32
  signal(SIGINT, (void (__cdecl *)(int)) intr_hdlr);
#else
  signal(SIGINT, intr_hdlr);
#endif

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  // Display libnfc version
  acLibnfcVersion = nfc_version();
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  // Try to open the NFC reader
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("Unable to open NFC device");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

  // Notes for ISO14443-A emulated tags:
  // * Only short UIDs are supported
  //   If your UID is longer it will be truncated
  //   Therefore e.g. an UltraLight can only have short UID, which is
  //   typically badly handled by readers who still try to send their "0x95"
  // * First byte of UID will be masked by 0x08 by the PN53x firmware
  //   as security countermeasure against real UID emulation

  // Example of a Mifare Classic Mini
  // Note that crypto1 is not implemented in this example
  nfc_target nt = {
    .nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_UNDEFINED,
    },
    .nti = {
      .nai = {
        .abtAtqa = { 0x00, 0x04 },
        .abtUid = { 0x08, 0xab, 0xcd, 0xef },
        .btSak = 0x09,
        .szUidLen = 4,
        .szAtsLen = 0,
      },
    },
  };
  /*
  // Example of a FeliCa
  nfc_target nt = {
    .nm = {
      .nmt = NMT_FELICA,
      .nbr = NBR_UNDEFINED,
    },
    .nti = {
      .nfi = {
        .abtId = { 0x01, 0xFE, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF },
        .abtPad = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF },
        .abtSysCode = { 0xFF, 0xFF },
      },
    },
  };
  */
  /*
  // Example of a ISO14443-4 (DESfire)
  nfc_target nt = {
    .nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_UNDEFINED,
    },
    .nti = {
      .nai = {
        abtAtqa = { 0x03, 0x44 },
        abtUid = { 0x08, 0xab, 0xcd, 0xef },
        btSak = 0x20,
        .szUidLen = 4,
        .abtAts = { 0x75, 0x77, 0x81, 0x02, 0x80 },
        .szAtsLen = 5,
      },
    },
  };
  */

  printf("%s will emulate this ISO14443-A tag:\n", argv[0]);
  print_nfc_target(&nt, true);

  // Switch off NP_EASY_FRAMING if target is not ISO14443-4
  nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, (nt.nti.nai.btSak & SAK_ISO14443_4_COMPLIANT));
  printf("NFC device (configured as target) is now emulating the tag, please touch it with a second NFC device (initiator)\n");
  if (!nfc_target_emulate_tag(pnd, &nt)) {
    nfc_perror(pnd, "nfc_target_emulate_tag");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}

