/**
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
 * 
 * 
 * @file nfcip-target.c
 * @brief
 */

#include <stdio.h>
#include <nfc.h>

int main(int argc, const char *argv[])
{
  byte_t abtRecv[MAX_FRAME_LEN];
  size_t szRecvBits;
  byte_t send[] = "Hello Mars!";
  dev_info *pdi = nfc_connect(NULL);

  if (!pdi || !nfc_target_init(pdi, abtRecv, &szRecvBits)) {
    printf("unable to connect or initialize\n");
    return 1;
  }

  if (!nfc_target_receive_dep_bytes(pdi, abtRecv, &szRecvBits)) {
    printf("unable to receive data\n");
    return 1;
  }
  abtRecv[szRecvBits] = 0;
  printf("Received: %s\n", abtRecv);
  printf("Sending : %s\n", send);

  if (!nfc_target_send_dep_bytes(pdi, send, 11)) {
    printf("unable to send data\n");
    return 1;
  }

  nfc_disconnect(pdi);
  return 0;
}
