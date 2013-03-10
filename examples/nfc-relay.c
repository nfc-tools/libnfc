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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <nfc/nfc.h>

#include "utils/nfc-utils.h"

#define MAX_FRAME_LEN 264
#define MAX_DEVICE_COUNT 2

static uint8_t abtReaderRx[MAX_FRAME_LEN];
static uint8_t abtReaderRxPar[MAX_FRAME_LEN];
static int szReaderRxBits;
static uint8_t abtTagRx[MAX_FRAME_LEN];
static uint8_t abtTagRxPar[MAX_FRAME_LEN];
static int szTagRxBits;
static nfc_device *pndReader;
static nfc_device *pndTag;
static bool quitting = false;

static void
intr_hdlr(int sig)
{
  (void) sig;
  printf("\nQuitting...\n");
  quitting = true;
  return;
}

static void
print_usage(char *argv[])
{
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tHelp. Print this message.\n");
  printf("\t-q\tQuiet mode. Suppress output of READER and EMULATOR data (improves timing).\n");
}

int
main(int argc, char *argv[])
{
  int     arg;
  bool    quiet_output = false;
  const char *acLibnfcVersion = nfc_version();

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      exit(EXIT_SUCCESS);
    } else if (0 == strcmp(argv[arg], "-q")) {
      quiet_output = true;
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      exit(EXIT_FAILURE);
    }
  }

  // Display libnfc version
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

#ifdef WIN32
  signal(SIGINT, (void (__cdecl *)(int)) intr_hdlr);
#else
  signal(SIGINT, intr_hdlr);
#endif

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }
  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  // List available devices
  size_t szFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

  if (szFound < 2) {
    ERR("%" PRIdPTR " device found but two opened devices are needed to relay NFC.", szFound);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  // Try to open the NFC emulator device
  pndTag = nfc_open(context, connstrings[0]);
  if (pndTag == NULL) {
    ERR("Error opening NFC emulator device");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("Hint: tag <---> initiator (relay) <---> target (relay) <---> original reader\n\n");

  printf("NFC emulator device: %s opened\n", nfc_device_get_name(pndTag));
  printf("[+] Try to break out the auto-emulation, this requires a second reader!\n");
  printf("[+] To do this, please send any command after the anti-collision\n");
  printf("[+] For example, send a RATS command or use the \"nfc-anticol\" tool\n");

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

  if ((szReaderRxBits = nfc_target_init(pndTag, &nt, abtReaderRx, sizeof(abtReaderRx), 0)) < 0) {
    ERR("%s", "Initialization of NFC emulator failed");
    nfc_close(pndTag);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("%s", "Configuring emulator settings...");
  if ((nfc_device_set_property_bool(pndTag, NP_HANDLE_CRC, false) < 0) ||
      (nfc_device_set_property_bool(pndTag, NP_HANDLE_PARITY, false) < 0) || (nfc_device_set_property_bool(pndTag, NP_ACCEPT_INVALID_FRAMES, true)) < 0) {
    nfc_perror(pndTag, "nfc_device_set_property_bool");
    nfc_close(pndTag);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("%s", "Done, emulated tag is initialized");

  // Try to open the NFC reader
  pndReader = nfc_open(context, connstrings[1]);
  if (pndReader == NULL) {
    printf("Error opening NFC reader device\n");
    nfc_close(pndTag);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader device: %s opened", nfc_device_get_name(pndReader));
  printf("%s", "Configuring NFC reader settings...");

  if (nfc_initiator_init(pndReader) < 0) {
    nfc_perror(pndReader, "nfc_initiator_init");
    nfc_close(pndTag);
    nfc_close(pndReader);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  if ((nfc_device_set_property_bool(pndReader, NP_HANDLE_CRC, false) < 0) ||
      (nfc_device_set_property_bool(pndReader, NP_HANDLE_PARITY, false) < 0) ||
      (nfc_device_set_property_bool(pndReader, NP_ACCEPT_INVALID_FRAMES, true)) < 0) {
    nfc_perror(pndReader, "nfc_device_set_property_bool");
    nfc_close(pndTag);
    nfc_close(pndReader);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("%s", "Done, relaying frames now!");

  while (!quitting) {
    // Test if we received a frame from the reader
    if ((szReaderRxBits = nfc_target_receive_bits(pndTag, abtReaderRx, sizeof(abtReaderRx), abtReaderRxPar)) > 0) {
      // Drop down the field before sending a REQA command and start a new session
      if (szReaderRxBits == 7 && abtReaderRx[0] == 0x26) {
        // Drop down field for a very short time (original tag will reboot)
        if (nfc_device_set_property_bool(pndReader, NP_ACTIVATE_FIELD, false) < 0) {
          nfc_perror(pndReader, "nfc_device_set_property_bool");
          nfc_close(pndTag);
          nfc_close(pndReader);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
        if (!quiet_output)
          printf("\n");
        if (nfc_device_set_property_bool(pndReader, NP_ACTIVATE_FIELD, true) < 0) {
          nfc_perror(pndReader, "nfc_device_set_property_bool");
          nfc_close(pndTag);
          nfc_close(pndReader);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
      }
      // Print the reader frame to the screen
      if (!quiet_output) {
        printf("R: ");
        print_hex_par(abtReaderRx, (size_t) szReaderRxBits, abtReaderRxPar);
      }
      // Forward the frame to the original tag
      if ((szTagRxBits = nfc_initiator_transceive_bits
                         (pndReader, abtReaderRx, (size_t) szReaderRxBits, abtReaderRxPar, abtTagRx, sizeof(abtTagRx), abtTagRxPar)) > 0) {
        // Redirect the answer back to the reader
        if (nfc_target_send_bits(pndTag, abtTagRx, szTagRxBits, abtTagRxPar) < 0) {
          nfc_perror(pndTag, "nfc_target_send_bits");
          nfc_close(pndTag);
          nfc_close(pndReader);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
        // Print the tag frame to the screen
        if (!quiet_output) {
          printf("T: ");
          print_hex_par(abtTagRx, szTagRxBits, abtTagRxPar);
        }
      }
    }
  }

  nfc_close(pndTag);
  nfc_close(pndReader);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
