/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * @file nfcip-target.c
 * @brief Turns the NFC device into a D.E.P. target (see NFCIP-1)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#include <nfc/nfc.h>

#define MAX_FRAME_LEN 264

int
main (int argc, const char *argv[])
{
  byte_t  abtRx[MAX_FRAME_LEN];
  size_t  szRx;
  size_t  szDeviceFound;
  byte_t  abtTx[] = "Hello Mars!";
  nfc_device_t *pnd;
  #define MAX_DEVICE_COUNT 2
  nfc_device_desc_t pnddDevices[MAX_DEVICE_COUNT];
  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szDeviceFound);
  // Little hack to allow using nfcip-initiator & nfcip-target from
  // the same machine: if there is more than one readers connected
  // nfcip-target will connect to the second reader
  // (we hope they're always detected in the same order)
  if (szDeviceFound == 1) {
    pnd = nfc_connect (&(pnddDevices[0]));
  } else if (szDeviceFound > 1) {
    pnd = nfc_connect (&(pnddDevices[1]));
  } else {
    printf("No device found.");
    return EXIT_FAILURE;
  }

  if (argc > 1) {
    printf ("Usage: %s\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Note: We have to build a "fake" nfc_target_t in order to do exactly the same that was done before the new nfc_target_init() was introduced.
  nfc_target_t nt = {
    .ntt = NTT_GENERIC_PASSIVE_106,
    .nti.nai.abtAtqa = "\x04\x00",
    .nti.nai.abtUid = "\xde\xad\xbe\xaf\x62",
    .nti.nai.btSak = 0x20,
    .nti.nai.szUidLen = 5,
    .nti.nai.szAtsLen = 0,
  };

  if (!pnd) {
    printf("Unable to connect to NFC device.\n");
    return EXIT_FAILURE;
  }
  printf ("Connected to NFC device: %s\n", pnd->acName);

  printf ("Waiting for initiator request...\n");
  if(!nfc_target_init (pnd, NTM_DEP, nt, abtRx, &szRx)) {
    nfc_perror(pnd, "nfc_target_init");
    return EXIT_FAILURE;
  }

  printf("Initiator request received. Waiting for data...\n");
  if (!nfc_target_receive_bytes (pnd, abtRx, &szRx)) {
    nfc_perror(pnd, "nfc_target_receive_bytes");
    return EXIT_FAILURE;
  }
  abtRx[szRx] = '\0';
  printf ("Received: %s\n", abtRx);

  printf ("Sending: %s\n", abtTx);
  if (!nfc_target_send_bytes (pnd, abtTx, sizeof(abtTx))) {
    nfc_perror(pnd, "nfc_target_send_bytes");
    return EXIT_FAILURE;
  }
  printf("Data sent.\n");

  nfc_disconnect (pnd);
  return EXIT_SUCCESS;
}
