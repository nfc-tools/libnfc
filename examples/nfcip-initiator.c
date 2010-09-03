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
 * @file nfcip-initiator.c
 * @brief
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <nfc/nfc.h>

#define MAX_FRAME_LEN 264

int main(int argc, const char *argv[])
{
  nfc_device_t *pnd;
  nfc_target_info_t ti;
  byte_t abtRecv[MAX_FRAME_LEN];
  size_t szRecvBits;
  byte_t send[] = "Hello World!";

  if (argc > 1) {
    errx (1, "usage: %s", argv[0]);
  }

  pnd = nfc_connect(NULL);
  if (!pnd || !nfc_initiator_init(pnd)
      || !nfc_initiator_select_dep_target(pnd, NM_PASSIVE_DEP, NULL, 0,
					  NULL, 0, NULL, 0, &ti)) {
    printf
	("unable to connect, initialize, or select the target\n");
    return 1;
  }

  printf("Sending : %s\n", send);
  if (!nfc_initiator_transceive_bytes(pnd,
					  send,
					  strlen((char*)send), abtRecv,
					  &szRecvBits)) {
    printf("unable to send data\n");
    return 1;
  }

  abtRecv[szRecvBits] = 0;
  printf("Received: %s\n", abtRecv);

  nfc_initiator_deselect_target(pnd);
  nfc_disconnect(pnd);
  return 0;
}
