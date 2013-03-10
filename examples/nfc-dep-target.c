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
 * @file nfc-dep-target.c
 * @brief Turns the NFC device into a D.E.P. target (see NFCIP-1)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <nfc/nfc.h>

#include "utils/nfc-utils.h"

#define MAX_FRAME_LEN 264

static nfc_device *pnd;
static nfc_context *context;

static void stop_dep_communication(int sig)
{
  (void) sig;
  if (pnd != NULL) {
    nfc_abort_command(pnd);
  } else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

int
main(int argc, const char *argv[])
{
  uint8_t  abtRx[MAX_FRAME_LEN];
  int  szRx;
  uint8_t  abtTx[] = "Hello Mars!";

  if (argc > 1) {
    printf("Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }
#define MAX_DEVICE_COUNT 2
  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
  // Little hack to allow using nfc-dep-initiator & nfc-dep-target from
  // the same machine: if there is more than one readers opened
  // nfc-dep-target will open the second reader
  // (we hope they're always detected in the same order)
  if (szDeviceFound == 1) {
    pnd = nfc_open(context, connstrings[0]);
  } else if (szDeviceFound > 1) {
    pnd = nfc_open(context, connstrings[1]);
  } else {
    printf("No device found.\n");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  nfc_target nt = {
    .nm = {
      .nmt = NMT_DEP,
      .nbr = NBR_UNDEFINED
    },
    .nti = {
      .ndi = {
        .abtNFCID3 = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0x00, 0x00 },
        .szGB = 4,
        .abtGB = { 0x12, 0x34, 0x56, 0x78 },
        .ndm = NDM_UNDEFINED,
        /* These bytes are not used by nfc_target_init: the chip will provide them automatically to the initiator */
        .btDID = 0x00,
        .btBS = 0x00,
        .btBR = 0x00,
        .btTO = 0x00,
        .btPP = 0x01,
      },
    },
  };

  if (pnd == NULL) {
    printf("Unable to open NFC device.\n");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

  signal(SIGINT, stop_dep_communication);

  printf("NFC device will now act as: ");
  print_nfc_target(&nt, false);

  printf("Waiting for initiator request...\n");
  if ((szRx = nfc_target_init(pnd, &nt, abtRx, sizeof(abtRx), 0)) < 0) {
    nfc_perror(pnd, "nfc_target_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("Initiator request received. Waiting for data...\n");
  if ((szRx = nfc_target_receive_bytes(pnd, abtRx, sizeof(abtRx), 0)) < 0) {
    nfc_perror(pnd, "nfc_target_receive_bytes");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  abtRx[(size_t) szRx] = '\0';
  printf("Received: %s\n", abtRx);

  printf("Sending: %s\n", abtTx);
  if (nfc_target_send_bytes(pnd, abtTx, sizeof(abtTx), 0) < 0) {
    nfc_perror(pnd, "nfc_target_send_bytes");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("Data sent.\n");

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
