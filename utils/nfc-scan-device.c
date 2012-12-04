/*-
 * Public platform independent Near Field Communication (NFC) library examples
 *
 * Copyright (C) 2009 Roel Verdult
 * Copyright (C) 2010 Romain Tartière
 * Copyright (C) 2010, 2011, 2012 Romuald Conty
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
 * @file nfc-scan-device.c
 * @brief Lists each available NFC device
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LIBUSB
#  ifdef DEBUG
#    include <usb.h>
#  endif
#endif

#include <err.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 16

static nfc_device *pnd;

static void
print_usage(const char *argv[])
{
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tPrint this help message.\n");
  printf("\t-v\tSet verbose display.\n");
  printf("\t-i\tAllow intrusive scan.\n");
}

int
main(int argc, const char *argv[])
{
  const char *acLibnfcVersion;
  size_t  i;
  bool verbose = false;

  nfc_context *context;
  nfc_init(&context);

  // Display libnfc version
  acLibnfcVersion = nfc_version();
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  // Get commandline options
  for (int arg = 1; arg < argc; arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      return EXIT_SUCCESS;
    } else if (0 == strcmp(argv[arg], "-v")) {
      verbose = true;
    } else if (0 == strcmp(argv[arg], "-i")) {
      setenv("LIBNFC_INTRUSIVE_SCAN", "yes", 1);
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      return EXIT_FAILURE;
    }
  }

#ifdef HAVE_LIBUSB
#  ifdef DEBUG
  usb_set_debug(4);
#  endif
#endif

  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

  int res = EXIT_FAILURE;
  if (szDeviceFound == 0) {
    printf("No NFC device found.\n");
    goto bye;
  }

  printf("%d NFC device(s) found:\n", (int)szDeviceFound);
  char *strinfo = NULL;
  for (i = 0; i < szDeviceFound; i++) {
    pnd = nfc_open(context, connstrings[i]);
    if (pnd != NULL) {
      printf("- %s:\n    %s\n", nfc_device_get_name(pnd), nfc_device_get_connstring(pnd));
      if (verbose) {
        if (nfc_device_get_information_about(pnd, &strinfo) >= 0) {
          printf("%s", strinfo);
          free(strinfo);
        }
      }
      nfc_close(pnd);
    } else {
      printf("nfc_open failed for %s\n", connstrings[i]);
    }
  }
  res = EXIT_SUCCESS;

bye:
  nfc_exit(context);
  return res;
}
