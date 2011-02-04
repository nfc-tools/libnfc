/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romuald Conty
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
 * @file nfc-emulate-uid.c
 * @brief Emulates a tag which which have a "really" custom UID
 * 
 * NFC devices are able to emulate passive tags but manufacturers restrict the
 * customization of UID. With PN53x, UID is only 4-byte long and the first
 * byte of emulated UID is hard-wired to 0x08 which is the standard way to say
 * this is a random UID.  This example shows how to emulate a fully customized
 * UID by "manually" replying to anti-collision process sent by the initiator.
 */

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

#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

static byte_t abtRecv[MAX_FRAME_LEN];
static size_t szRecvBits;
static nfc_device_t *pnd;

// ISO14443A Anti-Collision response
byte_t  abtAtqa[2] = { 0x04, 0x00 };
byte_t  abtUidBcc[5] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x62 };
byte_t  abtSak[9] = { 0x08, 0xb6, 0xdd };

void
intr_hdlr (void)
{
  printf ("\nQuitting...\n");
  if (pnd != NULL) {
    nfc_disconnect(pnd);
  }
  exit (EXIT_FAILURE);
}

void
print_usage (char *argv[])
{
  printf ("Usage: %s [OPTIONS] [UID]\n", argv[0]);
  printf ("Options:\n");
  printf ("\t-h\tHelp. Print this message.\n");
  printf ("\t-q\tQuiet mode. Silent output: received and sent frames will not be shown (improves timing).\n");
  printf ("\n");
  printf ("\t[UID]\tUID to emulate, specified as 8 HEX digits (default is DEADBEEF).\n");
}

int
main (int argc, char *argv[])
{
  byte_t *pbtTx = NULL;
  size_t  szTxBits;
  bool    quiet_output = false;

  int     arg,
          i;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp (argv[arg], "-h")) {
      print_usage (argv);
      exit(EXIT_SUCCESS);
    } else if (0 == strcmp (argv[arg], "-q")) {
      printf ("Quiet mode.\n");
      quiet_output = true;
    } else if ((arg == argc - 1) && (strlen (argv[arg]) == 8)) {        // See if UID was specified as HEX string
      byte_t  abtTmp[3] = { 0x00, 0x00, 0x00 };
      printf ("[+] Using UID: %s\n", argv[arg]);
      abtUidBcc[4] = 0x00;
      for (i = 0; i < 4; ++i) {
        memcpy (abtTmp, argv[arg] + i * 2, 2);
        abtUidBcc[i] = (byte_t) strtol ((char *) abtTmp, NULL, 16);
        abtUidBcc[4] ^= abtUidBcc[i];
      }
    } else {
      ERR ("%s is not supported option.", argv[arg]);
      print_usage (argv);
      exit(EXIT_FAILURE);
    }
  }

#ifdef WIN32
  signal (SIGINT, (void (__cdecl *) (int)) intr_hdlr);
#else
  signal (SIGINT, (void (*)()) intr_hdlr);
#endif

  // Try to open the NFC device
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    printf ("Unable to connect to NFC device\n");
    exit(EXIT_FAILURE);
  }

  printf ("\n");
  printf ("Connected to NFC device: %s\n", pnd->acName);
  printf ("[+] Try to break out the auto-emulation, this requires a second NFC device!\n");
  printf ("[+] To do this, please send any command after the anti-collision\n");
  printf ("[+] For example, send a RATS command or use the \"nfc-anticol\" or \"nfc-list\" tool.\n");

  // Note: We have to build a "fake" nfc_target_t in order to do exactly the same that was done before the new nfc_target_init() was introduced.
  nfc_target_t nt = {
    .nm.nmt = NMT_ISO14443A,
    .nm.nbr = NBR_UNDEFINED,
    .nti.nai.abtAtqa = { 0x04, 0x00 },
    .nti.nai.abtUid = { 0x08, 0xad, 0xbe, 0xef },
    .nti.nai.btSak = 0x20,
    .nti.nai.szUidLen = 4,
    .nti.nai.szAtsLen = 0,
  };
  if (!nfc_target_init (pnd, &nt, abtRecv, &szRecvBits)) {
    nfc_perror (pnd, "nfc_target_init");
    ERR ("Could not come out of auto-emulation, no command was received");
    exit(EXIT_FAILURE);
  }
  printf ("[+] Received initiator command: ");
  print_hex_bits (abtRecv, szRecvBits);
  printf ("[+] Configuring communication\n");
  if (!nfc_configure (pnd, NDO_HANDLE_CRC, false) || !nfc_configure (pnd, NDO_HANDLE_PARITY, true)) {
    nfc_perror (pnd, "nfc_configure");
    exit (EXIT_FAILURE);
  }
  printf ("[+] Done, the emulated tag is initialized with UID: %02X%02X%02X%02X\n\n", abtUidBcc[0], abtUidBcc[1],
          abtUidBcc[2], abtUidBcc[3]);

  while (true) {
    // Test if we received a frame
    if (nfc_target_receive_bits (pnd, abtRecv, &szRecvBits, NULL)) {
      // Prepare the command to send back for the anti-collision request
      switch (szRecvBits) {
      case 7:                  // Request or Wakeup
        pbtTx = abtAtqa;
        szTxBits = 16;
        // New anti-collsion session started
        if (!quiet_output)
          printf ("\n");
        break;

      case 16:                 // Select All
        pbtTx = abtUidBcc;
        szTxBits = 40;
        break;

      case 72:                 // Select Tag
        pbtTx = abtSak;
        szTxBits = 24;
        break;

      default:                 // unknown length?
        szTxBits = 0;
        break;
      }

      if (!quiet_output) {
        printf ("R: ");
        print_hex_bits (abtRecv, szRecvBits);
      }
      // Test if we know how to respond
      if (szTxBits) {
        // Send and print the command to the screen
        if (!nfc_target_send_bits (pnd, pbtTx, szTxBits, NULL)) {
          nfc_perror (pnd, "nfc_target_send_bits");
          exit (EXIT_FAILURE);
        }
        if (!quiet_output) {
          printf ("T: ");
          print_hex_bits (pbtTx, szTxBits);
        }
      }
    }
  }

  nfc_disconnect (pnd);
  exit (EXIT_SUCCESS);
}
