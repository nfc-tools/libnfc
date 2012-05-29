/*-
 * Public platform independent Near Field Communication (NFC) library examples
 *
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
 * @file pn53x-diagnose.c
 * @brief Small application to diagnose PN53x using dedicated commands
 */

/* vim:set ts=2 sw=2 et: */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>

#include "utils/nfc-utils.h"
#include "libnfc/chips/pn53x.h"

#define MAX_DEVICE_COUNT 16

int
main(int argc, const char *argv[])
{
  size_t  i;
  nfc_device *pnd;
  const char *acLibnfcVersion;
  bool    result;
  int res = 0;

  uint8_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof(abtRx);
  const uint8_t pncmd_diagnose_communication_line_test[] = { Diagnose, 0x00, 0x06, 'l', 'i', 'b', 'n', 'f', 'c' };
  const uint8_t pncmd_diagnose_rom_test[] = { Diagnose, 0x01 };
  const uint8_t pncmd_diagnose_ram_test[] = { Diagnose, 0x02 };

  if (argc > 1) {
    errx(1, "usage: %s", argv[0]);
  }

  nfc_init(NULL);

  // Display libnfc version
  acLibnfcVersion = nfc_version();
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  size_t szFound = nfc_list_devices(NULL, connstrings, MAX_DEVICE_COUNT);

  if (szFound == 0) {
    printf("No NFC device found.\n");
  }

  for (i = 0; i < szFound; i++) {
    pnd = nfc_open(NULL, connstrings[i]);

    if (pnd == NULL) {
      ERR("%s", "Unable to open NFC device.");
      return EXIT_FAILURE;
    }

    printf("NFC device [%s] opened.\n", nfc_device_get_name(pnd));

    res = pn53x_transceive(pnd, pncmd_diagnose_communication_line_test, sizeof(pncmd_diagnose_communication_line_test), abtRx, szRx, 0);
    if (res > 0) {
      szRx = (size_t) res;
      // Result of Diagnose ping for RC-S360 doesn't contain status byte so we've to handle both cases
      result = (memcmp(pncmd_diagnose_communication_line_test + 1, abtRx, sizeof(pncmd_diagnose_communication_line_test) - 1) == 0) ||
               (memcmp(pncmd_diagnose_communication_line_test + 2, abtRx, sizeof(pncmd_diagnose_communication_line_test) - 2) == 0);
      printf(" Communication line test: %s\n", result ? "OK" : "Failed");
    } else {
      nfc_perror(pnd, "pn53x_transceive: cannot diagnose communication line");
    }

    res = pn53x_transceive(pnd, pncmd_diagnose_rom_test, sizeof(pncmd_diagnose_rom_test), abtRx, szRx, 0);
    if (res > 0) {
      szRx = (size_t) res;
      result = ((szRx == 1) && (abtRx[0] == 0x00));
      printf(" ROM test: %s\n", result ? "OK" : "Failed");
    } else {
      nfc_perror(pnd, "pn53x_transceive: cannot diagnose ROM");
    }

    res = pn53x_transceive(pnd, pncmd_diagnose_ram_test, sizeof(pncmd_diagnose_ram_test), abtRx, szRx, 0);
    if (res > 0) {
      szRx = (size_t) res;
      result = ((szRx == 1) && (abtRx[0] == 0x00));
      printf(" RAM test: %s\n", result ? "OK" : "Failed");
    } else {
      nfc_perror(pnd, "pn53x_transceive: cannot diagnose RAM");
    }
  }
}
