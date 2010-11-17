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
  nfc_target_t nt;
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
  printf ("Connected to NFC device: %s\n", pnd->acName);

  if (!nfc_initiator_init (pnd)) {
    nfc_perror(pnd, "nfc_initiator_init");
    return EXIT_FAILURE;
  }

  if(!nfc_initiator_select_dep_target (pnd, NDM_PASSIVE, NBR_212, NULL, &nt)) {
    nfc_perror(pnd, "nfc_initiator_select_dep_target");
    return EXIT_FAILURE;
  }
  print_nfc_target (nt, false);

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
