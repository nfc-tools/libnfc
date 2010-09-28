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
 * @file nfc-emulate.c
 * @brief
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nfc/nfc.h>

#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

static byte_t abtRx[MAX_FRAME_LEN];
static size_t szRxLen;
static nfc_device_t *pnd;
static bool quiet_output = false;

#define SYMBOL_PARAM_fISO14443_4_PICC   0x20

bool
transmit_bytes (const byte_t * pbtTx, const size_t szTxLen)
{
  // Show transmitted command
  if (!quiet_output) {
    printf ("Tx: ");
    print_hex (pbtTx, szTxLen);
  }

  // Transmit the command bytes
  pn53x_set_parameters(pnd,0);
  if (!nfc_target_send_bytes(pnd, pbtTx, szTxLen))
    return false;
  pn53x_set_parameters(pnd,SYMBOL_PARAM_fISO14443_4_PICC);

  if (!pn53x_target_receive_bytes(pnd,abtRx,&szRxLen))
    return false;

  // Show received answer
  if (!quiet_output) {
    printf ("Rx: ");
    print_hex (abtRx, szRxLen);
  }
  // Succesful transfer
  return true;
}

int
main (int argc, char *argv[])
{
  // Try to open the NFC reader
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    printf ("Error connecting NFC reader\n");
    return 1;
  }

  printf ("[+] Connected to NFC reader: %s\n", pnd->acName);
  printf ("[+] Emulating NDEF tag now, please touch it with a second NFC device\n");
  if (!nfc_target_init (pnd, NTM_PICC, abtRx, &szRxLen)) {
    printf ("Error: Could not come out of auto-emulation, no command was received\n");
    return 1;
  }

  transmit_bytes("\x0a\x00\x6a\x87",4);
  transmit_bytes("\x0b\x00\x6a\x87",4);
  transmit_bytes("\x0a\x00\x90\x00",4);
  transmit_bytes("\x0b\x00\x90\x00",4);
  transmit_bytes("\x0a\x00\x00\x0f\x10\x00\x3b\x00\x34\x04\x06\xe1\x04\x0e\xe0\x00\x00\x90\x00",19);
  transmit_bytes("\x0b\x00\x90\x00",4);
  transmit_bytes("\x0a\x00\x00\x21\x90\x00",6);
  transmit_bytes("\x0b\x00\xd1\x02\x1c\x53\x70\x91\x01\x09\x54\x02\x65\x6e\x4c\x69\x62\x6e\x66\x63\x51\x01\x0b\x55\x03\x6c\x69\x62\x6e\x66\x63\x2e\x6f\x72\x67\x90\x00",37);
  transmit_bytes("\xca\x00",2);

  nfc_disconnect(pnd);
  exit (EXIT_SUCCESS);
}

