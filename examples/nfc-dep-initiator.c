/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * @file nfc-dep-initiator.c
 * @brief Turns the NFC device into a D.E.P. initiator (see NFCIP-1)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

int
main (int argc, const char *argv[])
{
  nfc_device_t *pnd;
  nfc_target_info_t nti;
  byte_t  abtRx[MAX_FRAME_LEN];
  size_t  szRx;
  byte_t  abtTx[] = "Hello World!";

  if (argc > 1) {
    printf ("Usage: %s\n", argv[0]);
    return EXIT_FAILURE;
  }

  pnd = nfc_connect (NULL);
  if (!pnd) {
    printf("Unable to connect to NFC device.\n");
    return EXIT_FAILURE;
  }

  if (!nfc_initiator_init (pnd)) {
    nfc_perror(pnd, "nfc_initiator_init");
    return EXIT_FAILURE;
  }

  if(!nfc_initiator_select_dep_target (pnd, NM_PASSIVE_DEP, NULL, 0, NULL, 0, NULL, 0, &nti)) {
    nfc_perror(pnd, "nfc_initiator_select_dep_target");
    return EXIT_FAILURE;
  }
  printf( "This D.E.P. target have been found:\n" );
  print_nfc_dep_info (nti.ndi);

  printf ("Sending: %s\n", abtTx);
  if (!nfc_initiator_transceive_bytes (pnd, abtTx, sizeof(abtTx), abtRx, &szRx)) {
    nfc_perror(pnd, "nfc_initiator_transceive_bytes");
    return EXIT_FAILURE;
  }

  abtRx[szRx] = 0;
  printf ("Received: %s\n", abtRx);

  nfc_initiator_deselect_target (pnd);
  nfc_disconnect (pnd);
  return EXIT_SUCCESS;
}
