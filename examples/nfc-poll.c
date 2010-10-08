/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Romuald Conty
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
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
  nfc_device_desc_t *pnddDevices;

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version ();

  if (argc > 1) {
    errx (1, "usage: %s", argv[0]);
  }

  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
    fprintf (stderr, "malloc() failed\n");
    return EXIT_FAILURE;
  }

  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (szFound == 0) {
    INFO ("%s", "No device found.");
  }

  for (i = 0; i < szFound; i++) {

    const byte_t btPollNr = 20;
    const byte_t btPeriod = 2;
    const nfc_target_type_t nttArray[5] = {NTT_GENERIC_PASSIVE_106, NTT_GENERIC_PASSIVE_212, NTT_GENERIC_PASSIVE_424, NTT_ISO14443B_106, NTT_JEWEL_106};
    const size_t szTargetTypes = 5;

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

    printf ("PN532 will poll during %ld ms\n", (unsigned long) btPollNr * szTargetTypes * btPeriod * 150);
    res = nfc_initiator_poll_targets (pnd, nttArray, szTargetTypes, btPollNr, btPeriod, antTargets, &szTargetFound);
    if (res) {
      uint8_t n;
      printf ("%ld target(s) have been found.\n", (unsigned long) szTargetFound);
      for (n = 0; n < szTargetFound; n++) {
        printf ("T%d: targetType=%02x ", n + 1, antTargets[n].ntt);
        switch(antTargets[n].ntt) {
          case NTT_JEWEL_106:
            printf ("(Innovision Jewel tag), targetData:\n");
            print_nfc_jewel_info (antTargets[n].nti.nji);
            break;
          case NTT_MIFARE:
            printf ("(Mifare card), targetData:\n");
            print_nfc_iso14443a_info (antTargets[n].nti.nai);
            break;
          case NTT_FELICA_212:
            printf ("(FeliCa 212 kbps card), targetData:\n");
            print_nfc_felica_info (antTargets[n].nti.nfi);
            break;
          case NTT_FELICA_424:
            printf ("(FeliCa 212 kbps card), targetData:\n");
            print_nfc_felica_info (antTargets[n].nti.nfi);
            break;
          case NTT_ISO14443A_106:
            printf ("(Passive 106 kbps ISO/IEC 14443-4A card), targetData:\n");
            print_nfc_iso14443a_info (antTargets[n].nti.nai);
            break;
          case NTT_ISO14443B_TCL_106:
            printf ("(Passive 106 kbps ISO/IEC 14443-4B card), targetData:\n");
            print_nfc_iso14443b_info (antTargets[n].nti.nbi);
            break;
          case NTT_DEP_PASSIVE_106:
            printf ("(DEP passive 106 kbps)\n");
            break;
          case NTT_DEP_PASSIVE_212:
            printf ("(DEP passive 212 kbps)\n");
            break;
          case NTT_DEP_PASSIVE_424:
            printf ("(DEP passive 424 kbps)\n");
            break;
          case NTT_DEP_ACTIVE_106:
            printf ("(DEP active 106 kbps)\n");
            break;
          case NTT_DEP_ACTIVE_212:
            printf ("(DEP active 212 kbps)\n");
            break;
          case NTT_DEP_ACTIVE_424:
            printf ("(DEP active 424 kbps)\n");
            break;
        };
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
