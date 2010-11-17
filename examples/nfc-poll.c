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
 * @file nfc-poll
 * @brief Polling example
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <err.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>
#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_DEVICE_COUNT 16

static nfc_device_t *pnd;

int
main (int argc, const char *argv[])
{
  size_t  szFound;
  size_t  i;
  bool verbose = false;
  nfc_device_desc_t *pnddDevices;

  pnddDevices = parse_args (argc, argv, &szFound, &verbose);

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version ();

  if (argc > 1) {
    errx (1, "usage: %s", argv[0]);
  }

  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  if (szFound == 0) {
    if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
      fprintf (stderr, "malloc() failed\n");
      return EXIT_FAILURE;
    }
  }

  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (szFound == 0) {
    printf ("No NFC device found.\n");
  }

  for (i = 0; i < szFound; i++) {

    const byte_t btPollNr = 20;
    const byte_t btPeriod = 2;
    const nfc_modulation_t nmModulations[5] = {
      { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
      { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
      { .nmt = NMT_FELICA, .nbr = NBR_212 },
      { .nmt = NMT_FELICA, .nbr = NBR_424 },
      { .nmt = NMT_JEWEL, .nbr = NBR_106 },
    };
    const size_t szModulations = 5;

    nfc_target_t antTargets[2];
    size_t  szTargetFound;
    bool    res;

    pnd = nfc_connect (&(pnddDevices[i]));

    if (pnd == NULL) {
      ERR ("%s", "Unable to connect to NFC device.");
      return 1;
    }
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

    printf ("Connected to NFC reader: %s\n", pnd->acName);

    printf ("PN532 will poll during %ld ms\n", (unsigned long) btPollNr * szModulations * btPeriod * 150);
    res = nfc_initiator_poll_targets (pnd, nmModulations, szModulations, btPollNr, btPeriod, antTargets, &szTargetFound);
    if (res) {
      uint8_t n;
      printf ("%ld target(s) have been found.\n", (unsigned long) szTargetFound);
      for (n = 0; n < szTargetFound; n++) {
        printf ("T%d: ", n + 1);
        print_nfc_target ( antTargets[n], verbose );

      }
    } else {
      nfc_perror (pnd, "nfc_initiator_poll_targets");
      nfc_disconnect (pnd);
      exit (EXIT_FAILURE);
    }
    nfc_disconnect (pnd);
  }

  free (pnddDevices);
  return 0;
}
