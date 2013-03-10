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
 * @file nfc-dep-initiator.c
 * @brief Turns the NFC device into a D.E.P. initiator (see NFCIP-1)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  nfc_target nt;
  uint8_t  abtRx[MAX_FRAME_LEN];
  uint8_t  abtTx[] = "Hello World!";

  if (argc > 1) {
    printf("Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  pnd = nfc_open(context, NULL);
  if (pnd == NULL) {
    ERR("Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("NFC device: %s\n opened", nfc_device_get_name(pnd));

  signal(SIGINT, stop_dep_communication);

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_select_dep_target(pnd, NDM_PASSIVE, NBR_212, NULL, &nt, 1000) < 0) {
    nfc_perror(pnd, "nfc_initiator_select_dep_target");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  print_nfc_target(&nt, false);

  printf("Sending: %s\n", abtTx);
  int res;
  if ((res = nfc_initiator_transceive_bytes(pnd, abtTx, sizeof(abtTx), abtRx, sizeof(abtRx), 0)) < 0) {
    nfc_perror(pnd, "nfc_initiator_transceive_bytes");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  abtRx[res] = 0;
  printf("Received: %s\n", abtRx);

  if (nfc_initiator_deselect_target(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_deselect_target");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
