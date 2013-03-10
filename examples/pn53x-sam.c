/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Additional contributors of this file:
 * Copyright (C) 2010      Emanuele Bertoldi
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
 * @file pn53x-sam.c
 * @brief Configures the NFC device to communicate with a SAM (Secure Access Module).
 * @note This example requiers a PN532 with SAM connected using S2C interface
 * @see PN532 User manual
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <nfc/nfc.h>

#include "utils/nfc-utils.h"
#include "libnfc/chips/pn53x.h"

#define MAX_FRAME_LEN 264
#define TIMEOUT 60              // secs.

static void
wait_one_minute(void)
{
  int     secs = 0;

  printf("|");
  fflush(stdout);

  while (secs < TIMEOUT) {
    sleep(1);
    secs++;
    printf(".");
    fflush(stdout);
  }

  printf("|\n");
}

int
main(int argc, const char *argv[])
{
  (void) argc;
  (void) argv;

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version();
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  // Open using the first available NFC device
  nfc_device *pnd;
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

  // Print the example's menu
  printf("\nSelect the communication mode:\n");
  printf("[1] Virtual card mode.\n");
  printf("[2] Wired card mode.\n");
  printf("[3] Dual card mode.\n");
  printf(">> ");

  // Take user's choice
  char    input = getchar();
  printf("\n");
  if ((input < '1') || (input > '3')) {
    ERR("%s", "Invalid selection.");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  /*
   * '1' -> "Virtual mode" (0x02)
   * '2' -> "Wired card" (0x03)
   * '3' -> "Dual card" (0x04)
   */
  int iMode = input - '0' + 0x01;
  pn532_sam_mode mode = iMode;

  // Connect with the SAM

  switch (mode) {
    case PSM_VIRTUAL_CARD: {
      // FIXME Its a private pn53x function
      if (pn532_SAMConfiguration(pnd, mode, 0) < 0) {
        nfc_perror(pnd, "pn53x_SAMConfiguration");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      printf("Now the SAM is readable for 1 minute from an external reader.\n");
      wait_one_minute();
    }
    break;

    case PSM_WIRED_CARD: {
      // Set opened NFC device to initiator mode
      if (nfc_initiator_init_secure_element(pnd) < 0) {
        nfc_perror(pnd, "nfc_initiator_init_secure_element");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }

      // Let the reader only try once to find a tag
      if (nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false) < 0) {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      // Read the SAM's info
      const nfc_modulation nmSAM = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
      };
      nfc_target nt;

      int res;
      if ((res = nfc_initiator_select_passive_target(pnd, nmSAM, NULL, 0, &nt)) < 0) {
        nfc_perror(pnd, "nfc_initiator_select_passive_target");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      } else if (res == 0) {
        ERR("No SAM found.");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      } else if (res == 1) {
        printf("The following ISO14443A tag (SAM) was found:\n");
        print_nfc_target(&nt, true);
      } else {
        ERR("%s", "More than one ISO14442 tag found as SAM.");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
    }
    break;

    case PSM_DUAL_CARD: {
      // FIXME Its a private pn53x function
      if (pn532_SAMConfiguration(pnd, mode, 0) < 0) {
        nfc_perror(pnd, "pn53x_SAMConfiguration");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      uint8_t  abtRx[MAX_FRAME_LEN];

      nfc_target nt = {
        .nm = {
          .nmt = NMT_ISO14443A,
          .nbr = NBR_UNDEFINED,
        },
        .nti = {
          .nai = {
            .abtAtqa = { 0x04, 0x00 },
            .abtUid = { 0x08, 0xad, 0xbe, 0xef },
            .btSak = 0x20,
            .szUidLen = 4,
            .szAtsLen = 0,
          },
        },
      };
      printf("Now both, NFC device (configured as target) and SAM are readables from an external NFC initiator.\n");
      printf("Please note that NFC device (configured as target) stay in target mode until it receive RATS, ATR_REQ or proprietary command.\n");
      if (nfc_target_init(pnd, &nt, abtRx, sizeof(abtRx), 0) < 0) {
        nfc_perror(pnd, "nfc_target_init");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      // wait_one_minute ();
    }
    break;
    case PSM_NORMAL:
      // This should not happend... nothing to do.
      break;
  }

  // Disconnect from the SAM
  pn532_SAMConfiguration(pnd, PSM_NORMAL, -1);

  // Close NFC device
  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
