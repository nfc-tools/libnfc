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
 * @file nfc-relay.c
 * @brief Relay example using two devices.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <nfc/nfc.h>

#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264
#define MAX_DEVICE_COUNT 2

static byte_t abtReaderRx[MAX_FRAME_LEN];
static byte_t abtReaderRxPar[MAX_FRAME_LEN];
static size_t szReaderRxBits;
static byte_t abtTagRx[MAX_FRAME_LEN];
static byte_t abtTagRxPar[MAX_FRAME_LEN];
static size_t szTagRxBits;
static nfc_device_t *pndReader;
static nfc_device_t *pndTag;
static bool quitting = false;

void
intr_hdlr (void)
{
  printf ("\nQuitting...\n");
  quitting = true;
  return;
}

void
print_usage (char *argv[])
{
  printf ("Usage: %s [OPTIONS]\n", argv[0]);
  printf ("Options:\n");
  printf ("\t-h\tHelp. Print this message.\n");
  printf ("\t-q\tQuiet mode. Suppress output of READER and EMULATOR data (improves timing).\n");
}

int
main (int argc, char *argv[])
{
  int     arg;
  bool    quiet_output = false;
  size_t  szFound;
  nfc_device_desc_t *pnddDevices;
  const char *acLibnfcVersion = nfc_version ();

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp (argv[arg], "-h")) {
      print_usage (argv);
      return EXIT_SUCCESS;
    } else if (0 == strcmp (argv[arg], "-q")) {
      quiet_output = true;
    } else {
      ERR ("%s is not supported option.", argv[arg]);
      print_usage (argv);
      return EXIT_FAILURE;
    }
  }

  // Display libnfc version
  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

#ifdef WIN32
  signal (SIGINT, (void (__cdecl *) (int)) intr_hdlr);
#else
  signal (SIGINT, (void (*)()) intr_hdlr);
#endif

  // Allocate memory to put the result of available devices listing
  if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
    fprintf (stderr, "malloc() failed\n");
    return EXIT_FAILURE;
  }
  // List available devices
  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (szFound < 2) {
    ERR ("%zd device found but two connected devices are needed to relay NFC.", szFound);
    return EXIT_FAILURE;
  }
  // Try to open the NFC emulator device
  pndTag = nfc_connect (&(pnddDevices[0]));
  if (pndTag == NULL) {
    printf ("Error connecting NFC emulator device\n");
    return EXIT_FAILURE;
  }

  printf ("Hint: tag <---> initiator (relay) <---> target (relay) <---> original reader\n\n");

  printf ("Connected to the NFC emulator device: %s\n", pndTag->acName);
  printf ("[+] Try to break out the auto-emulation, this requires a second reader!\n");
  printf ("[+] To do this, please send any command after the anti-collision\n");
  printf ("[+] For example, send a RATS command or use the \"nfc-anticol\" tool\n");

  nfc_target_t nt = {
    .nm.nmt = NMT_ISO14443A,
    .nm.nbr = NBR_UNDEFINED,
    .nti.nai.abtAtqa = { 0x04, 0x00 },
    .nti.nai.abtUid = { 0xde, 0xad, 0xbe, 0xef },
    .nti.nai.btSak = 0x20,
    .nti.nai.szUidLen = 4,
    .nti.nai.szAtsLen = 0,
  };

  if (!nfc_target_init (pndTag, &nt, abtReaderRx, &szReaderRxBits)) {
    ERR ("%s", "Initialization of NFC emulator failed");
    nfc_disconnect (pndTag);
    return EXIT_FAILURE;
  }
  printf ("%s", "Configuring emulator settings...");
  if (!nfc_configure (pndTag, NDO_HANDLE_CRC, false) ||
      !nfc_configure (pndTag, NDO_HANDLE_PARITY, false) || !nfc_configure (pndTag, NDO_ACCEPT_INVALID_FRAMES, true)) {
    nfc_perror (pndTag, "nfc_configure");
    exit (EXIT_FAILURE);
  }
  printf ("%s", "Done, emulated tag is initialized");

  // Try to open the NFC reader
  pndReader = nfc_connect (&(pnddDevices[1]));

  printf ("Connected to the NFC reader device: %s", pndReader->acName);
  printf ("%s", "Configuring NFC reader settings...");
  nfc_initiator_init (pndReader);
  if (!nfc_configure (pndReader, NDO_HANDLE_CRC, false) ||
      !nfc_configure (pndReader, NDO_HANDLE_PARITY, false) ||
      !nfc_configure (pndReader, NDO_ACCEPT_INVALID_FRAMES, true)) {
    nfc_perror (pndReader, "nfc_configure");
    exit (EXIT_FAILURE);
  }
  printf ("%s", "Done, relaying frames now!");

  while (!quitting) {
    // Test if we received a frame from the reader
    if (nfc_target_receive_bits (pndTag, abtReaderRx, &szReaderRxBits, abtReaderRxPar)) {
      // Drop down the field before sending a REQA command and start a new session
      if (szReaderRxBits == 7 && abtReaderRx[0] == 0x26) {
        // Drop down field for a very short time (original tag will reboot)
        if (!nfc_configure (pndReader, NDO_ACTIVATE_FIELD, false)) {
          nfc_perror (pndReader, "nfc_configure");
          exit (EXIT_FAILURE);
        }
        if (!quiet_output)
          printf ("\n");
        if (!nfc_configure (pndReader, NDO_ACTIVATE_FIELD, true)) {
          nfc_perror (pndReader, "nfc_configure");
          exit (EXIT_FAILURE);
        }
      }
      // Print the reader frame to the screen
      if (!quiet_output) {
        printf ("R: ");
        print_hex_par (abtReaderRx, szReaderRxBits, abtReaderRxPar);
      }
      // Forward the frame to the original tag
      if (nfc_initiator_transceive_bits
          (pndReader, abtReaderRx, szReaderRxBits, abtReaderRxPar, abtTagRx, &szTagRxBits, abtTagRxPar)) {
        // Redirect the answer back to the reader
        if (!nfc_target_send_bits (pndTag, abtTagRx, szTagRxBits, abtTagRxPar)) {
          nfc_perror (pndTag, "nfc_target_send_bits");
          exit (EXIT_FAILURE);
        }
        // Print the tag frame to the screen
        if (!quiet_output) {
          printf ("T: ");
          print_hex_par (abtTagRx, szTagRxBits, abtTagRxPar);
        }
      }
    }
  }

  nfc_disconnect (pndTag);
  nfc_disconnect (pndReader);
  exit (EXIT_SUCCESS);
}
