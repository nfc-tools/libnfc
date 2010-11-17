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
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
// Needed by sleep() under Unix
#  include <unistd.h>
#  define sleep sleep
#  define SUSP_TIME 1           // secs.
#else
// Needed by Sleep() under Windows
#  include "../contrib/windows.h"
#  include <winbase.h>
#  define sleep Sleep
#  define SUSP_TIME 1000        // msecs.
#endif

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#include "chips/pn53x.h"

#define MAX_FRAME_LEN 264
#define TIMEOUT 60              // secs.

#define NORMAL_MODE 1
#define VIRTUAL_CARD_MODE 2
#define WIRED_CARD_MODE 3
#define DUAL_CARD_MODE 4

bool
sam_connection (nfc_device_t * pnd, int mode)
{
  byte_t  pncmd_sam_config[] = { 0xD4, 0x14, 0x00, 0x00 };
  size_t  szCmd = 0;

  byte_t  abtRx[MAX_FRAME_LEN];
  size_t  szRx;

  pncmd_sam_config[2] = mode;

  switch (mode) {
  case VIRTUAL_CARD_MODE:
    {
      // Only the VIRTUAL_CARD_MODE requires 4 bytes.
      szCmd = sizeof (pncmd_sam_config);
    }
    break;

  default:
    {
      szCmd = sizeof (pncmd_sam_config) - 1;
    }
    break;
  }

  if (!pn53x_transceive (pnd, pncmd_sam_config, szCmd, abtRx, &szRx)) {
    nfc_perror(pnd, "pn53x_transceive");
    ERR ("%s %d", "Unable to execute SAMConfiguration command with mode byte:", mode);
    return false;
  }

  return true;
}

void
wait_one_minute ()
{
  int     secs = 0;

  printf ("|");
  fflush (stdout);

  while (secs < TIMEOUT) {
    sleep (SUSP_TIME);
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
  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

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
  int     mode = input - '0' + 1;
  printf ("\n");
  if (mode < VIRTUAL_CARD_MODE || mode > DUAL_CARD_MODE) {
    ERR ("%s", "Invalid selection.");
    return EXIT_FAILURE;
  }
  // Connect with the SAM
  sam_connection (pnd, mode);

  switch (mode) {
  case VIRTUAL_CARD_MODE:
    {
      // FIXME after the loop the device doesn't respond to host commands...
      printf ("Now the SAM is readable for 1 minute from an external reader.\n");
      wait_one_minute ();
    }
    break;

  case WIRED_CARD_MODE:
    {
      nfc_target_t nt;

      // Set connected NFC device to initiator mode
      nfc_initiator_init (pnd);

      // Drop the field for a while
      if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, false)) {
        nfc_perror (pnd, "nfc_configure");
        exit (EXIT_FAILURE);
      }
      // Let the reader only try once to find a tag
      if (!nfc_configure (pnd, NDO_INFINITE_SELECT, false)) {
        nfc_perror (pnd, "nfc_configure");
        exit (EXIT_FAILURE);
      }
      // Enable field so more power consuming cards can power themselves up
      if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, true)) {
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
        return EXIT_FAILURE;
      }

      printf ("The following ISO14443A tag (SAM) was found:\n\n");
      print_nfc_iso14443a_info (nt.nti.nai, true);
    }
    break;

  case DUAL_CARD_MODE:
    {
      byte_t  abtRx[MAX_FRAME_LEN];
      size_t  szRx;

      nfc_target_t nt = {
        .nm.nmt = NMT_ISO14443A,
        .nm.nbr = NBR_UNDEFINED,
        .nti.nai.abtAtqa = { 0x04, 0x00 },
        .nti.nai.abtUid = { 0x08, 0xad, 0xbe, 0xef },
        .nti.nai.btSak = 0x20,
        .nti.nai.szUidLen = 4,
        .nti.nai.szAtsLen = 0,
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
  }

  // Disconnect from the SAM
  sam_connection (pnd, NORMAL_MODE);

  // Disconnect from NFC device
  nfc_disconnect (pnd);

  return EXIT_SUCCESS;
}
