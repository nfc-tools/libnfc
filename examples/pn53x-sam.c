/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2010, Emanuele Bertoldi
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

void
wait_one_minute (void)
{
  int     secs = 0;

  printf ("|");
  fflush (stdout);

  while (secs < TIMEOUT) {
    sleep (1);
    secs++;
    printf (".");
    fflush (stdout);
  }

  printf ("|\n");
}

int
main (int argc, const char *argv[])
{
  nfc_device_t *pnd;

  (void) argc;
  (void) argv;

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version ();
  printf ("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  // Connect using the first available NFC device
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    ERR ("%s", "Unable to connect to NFC device.");
    return EXIT_FAILURE;
  }

  printf ("Connected to NFC device: %s\n", pnd->acName);

  // Print the example's menu
  printf ("\nSelect the communication mode:\n");
  printf ("[1] Virtual card mode.\n");
  printf ("[2] Wired card mode.\n");
  printf ("[3] Dual card mode.\n");
  printf (">> ");

  // Take user's choice
  char    input = getchar ();
  int iMode = input - '0' + 1;
  printf ("\n");
  if (iMode < 1 || iMode > 3) {
    ERR ("%s", "Invalid selection.");
    exit (EXIT_FAILURE);
  }
  pn532_sam_mode mode = iMode;

  // Connect with the SAM
  // FIXME: Its a private pn53x function
  if (!pn53x_SAMConfiguration (pnd, mode, NULL)) {
    nfc_perror (pnd, "pn53x_SAMConfiguration");
    exit (EXIT_FAILURE);
  }

  switch (mode) {
  case PSM_VIRTUAL_CARD:
    {
      printf ("Now the SAM is readable for 1 minute from an external reader.\n");
      wait_one_minute ();
    }
    break;

  case PSM_WIRED_CARD:
    {
      nfc_target_t nt;

      // Set connected NFC device to initiator mode
      nfc_initiator_init (pnd);

      // Let the reader only try once to find a tag
      if (!nfc_configure (pnd, NDO_INFINITE_SELECT, false)) {
        nfc_perror (pnd, "nfc_configure");
        exit (EXIT_FAILURE);
      }
      // Read the SAM's info
      const nfc_modulation_t nmSAM = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
      };
      if (!nfc_initiator_select_passive_target (pnd, nmSAM, NULL, 0, &nt)) {
        nfc_perror (pnd, "nfc_initiator_select_passive_target");
        ERR ("%s", "Reading of SAM info failed.");
        exit (EXIT_FAILURE);
      }

      printf ("The following ISO14443A tag (SAM) was found:\n");
      print_nfc_iso14443a_info (nt.nti.nai, true);
    }
    break;

  case PSM_DUAL_CARD:
    {
      byte_t  abtRx[MAX_FRAME_LEN];
      size_t  szRx = sizeof(abtRx);

      nfc_target_t nt = {
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
      printf ("Now both, NFC device (configured as target) and SAM are readables from an external NFC initiator.\n");
      printf ("Please note that NFC device (configured as target) stay in target mode until it receive RATS, ATR_REQ or proprietary command.\n");
      if (!nfc_target_init (pnd, &nt, abtRx, &szRx)) {
        nfc_perror(pnd, "nfc_target_init");
        return EXIT_FAILURE;
      }
      // wait_one_minute ();
    }
    break;
  default:
    break;
  }

  // Disconnect from the SAM
  pn53x_SAMConfiguration (pnd, PSM_NORMAL, NULL);

  // Disconnect from NFC device
  nfc_disconnect (pnd);

  exit (EXIT_SUCCESS);
}
