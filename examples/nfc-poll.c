/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2010, Romuald Conty
 * Copyright (C) 2011, Romain Tartiere, Romuald Conty
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
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#include "utils/nfc-utils.h"

#define MAX_DEVICE_COUNT 16

static nfc_device_t *pnd = NULL;

void stop_polling (int sig)
{
  (void) sig;
  if (pnd)
    nfc_abort_command (pnd);
  else
    exit (EXIT_FAILURE);
}

int
main (int argc, const char *argv[])
{
  size_t  szFound;
  size_t  i;
  bool verbose = false;
  nfc_device_desc_t *pnddDevices;

  signal (SIGINT, stop_polling);

  pnddDevices = parse_args (argc, argv, &szFound, &verbose);

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version ();

  if (argc > 1) {
    errx (1, "usage: %s", argv[0]);
  }

  printf ("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  if (szFound == 0) {
    if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
      fprintf (stderr, "malloc() failed\n");
      exit (EXIT_FAILURE);
    }
  }

  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (szFound == 0) {
    printf ("No NFC device found.\n");
  }

  for (i = 0; i < szFound; i++) {
    const uint8_t uiPollNr = 20;
    const uint8_t uiPeriod = 2;
    const nfc_modulation_t nmModulations[5] = {
      { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
      { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
      { .nmt = NMT_FELICA, .nbr = NBR_212 },
      { .nmt = NMT_FELICA, .nbr = NBR_424 },
      { .nmt = NMT_JEWEL, .nbr = NBR_106 },
    };
    const size_t szModulations = 5;

    nfc_target_t nt;
    bool    res;

    pnd = nfc_connect (&(pnddDevices[i]));

    if (pnd == NULL) {
      ERR ("%s", "Unable to connect to NFC device.");
      exit (EXIT_FAILURE);
    }
    nfc_initiator_init (pnd);

    printf ("Connected to NFC reader: %s\n", pnd->acName);
    printf ("NFC device will poll during %ld ms (%u pollings of %lu ms for %zd modulations)\n", (unsigned long) uiPollNr * szModulations * uiPeriod * 150, uiPollNr, (unsigned long) uiPeriod * 150, szModulations);
    res = nfc_initiator_poll_target (pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt);
    if (res) {
      print_nfc_target ( nt, verbose );
    } else {
      if (pnd->iLastError) {
        nfc_perror (pnd, "nfc_initiator_poll_targets");
        nfc_disconnect (pnd);
        exit (EXIT_FAILURE);
      } else {
        printf ("No target found.\n");
      }
    }
    nfc_disconnect (pnd);
  }

  free (pnddDevices);
  exit (EXIT_SUCCESS);
}
