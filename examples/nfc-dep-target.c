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
 * @file nfc-dep-target.c
 * @brief Turns the NFC device into a D.E.P. target (see NFCIP-1)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

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
  // Little hack to allow using nfc-dep-initiator & nfc-dep-target from
  // the same machine: if there is more than one readers connected
  // nfc-dep-target will connect to the second reader
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

  nfc_target_t nt = {
    .nm.nmt = NMT_DEP,
    .nm.nbr = NBR_UNDEFINED, // Will be updated by nfc_target_init
    .nti.ndi.abtNFCID3 = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0x00, 0x00 },
    .nti.ndi.szGB = 4,
    .nti.ndi.abtGB = { 0x12, 0x34, 0x56, 0x78 },
    /* These bytes are not used by nfc_target_init: the chip will provide them automatically to the initiator */
    .nti.ndi.btDID = 0x00,
    .nti.ndi.btBS = 0x00,
    .nti.ndi.btBR = 0x00,
    .nti.ndi.btTO = 0x00,
    .nti.ndi.btPP = 0x01,
  };

  if (!pnd) {
    printf("Unable to connect to NFC device.\n");
    return EXIT_FAILURE;
  }
  printf ("Connected to NFC device: %s\n", pnd->acName);

  printf ("NFC device will now act as: ");
  print_nfc_target (nt, false);

  printf ("Waiting for initiator request...\n");
  if(!nfc_target_init (pnd, &nt, abtRx, &szRx)) {
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
