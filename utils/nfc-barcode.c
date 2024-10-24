/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2017 Philippe Teuwen
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
 * @file nfc-barcode.c
 * @brief Reads a NFC Barcode tag
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

static nfc_device *pnd;

bool    verbose = false;

static void
print_usage(char *argv[])
{
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tHelp. Print this message.\n");
  printf("\t-v\tVerbose mode.\n");
}


static bool
decode_barcode(uint8_t *pbtBarcode, const size_t szBarcode)
{
  if (verbose) {
    printf("Manufacturer ID field: %02X\n", pbtBarcode[0]);
    switch (pbtBarcode[0]) {
      case 0xb7:
        printf("Manufacturer: Thinfilm\n");
        break;
      default:
        printf("Manufacturer: unknown\n");
        break;
    }
  }
  if (verbose) {
    printf("Data Format Field: %02X\n", pbtBarcode[1]);
  }
  switch (pbtBarcode[1]) {
    case 0:
      printf("Data Format Field: Reserved for allocation by tag manufacturer\n");
      return false;
      break;
    case 1:
    case 2:
    case 3:
    case 4:
      switch (pbtBarcode[1]) {
        case 1:
          printf("http://www.");
          break;
        case 2:
          printf("https://www.");
          break;
        case 3:
          printf("http://");
          break;
        case 4:
          printf("https://");
          break;
      }
      for (uint8_t i = 2; i < 15; i++) {
        if ((pbtBarcode[i] == 0xfe) || (i == 14)) {
          pbtBarcode[i] = '\n';
          pbtBarcode[i + 1] = 0;
          break;
        }
      }
      printf("%s", (char *)pbtBarcode + 2);
      break;
    case 5:
      printf("EPC: ");
      for (uint8_t i = 0; i < 12; i++) {
        printf("%02x", pbtBarcode[i + 2]);
      }
      printf("\n");
      break;
    default:
      printf("Data Format Field: unknown (%02X)\n", pbtBarcode[1]);
      printf("Data:");
      for (uint8_t i = 2; i < szBarcode - 2; i++) {
        printf("%02x", pbtBarcode[i]);
      }
      printf("\n");
      break;
  }
  return true;
}

int
main(int argc, char *argv[])
{
  int     arg;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      exit(EXIT_SUCCESS);
    } else if (0 == strcmp(argv[arg], "-v")) {
      verbose = true;
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      exit(EXIT_FAILURE);
    }
  }

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  // Try to open the NFC reader
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("Error opening NFC reader");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  // Initialise NFC device as "initiator"
  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n\n", nfc_device_get_name(pnd));

  nfc_modulation nm;
  nm.nmt = NMT_BARCODE;
  nm.nbr = NBR_106;
  nfc_target ant[1];
  // List NFC Barcode targets
  if ((nfc_initiator_list_passive_targets(pnd, nm, ant, 1)) == 1) {
    if (verbose) {
      for (uint8_t i = 0; i < (&ant[0])->nti.nti.szDataLen; i++) {
        printf("%02x", (&ant[0])->nti.nti.abtData[i]);
      }
      printf("\n");
    }
    decode_barcode((&ant[0])->nti.nti.abtData, (&ant[0])->nti.nti.szDataLen);
  }
  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
