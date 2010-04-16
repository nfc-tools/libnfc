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
  #include "config.h"
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

static byte_t abtRecv[MAX_FRAME_LEN];
static size_t szRecvBits;
static nfc_device_t* pnd;

// ISO14443A Anti-Collision response
byte_t abtAtqa      [2] = { 0x04,0x00 };
byte_t abtUidBcc    [5] = { 0xDE,0xAD,0xBE,0xAF,0x62 };
byte_t abtSak       [9] = { 0x08,0xb6,0xdd };

void print_usage(char* argv[])
{
  printf("Usage: %s [OPTIONS] [UID]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tHelp. Print this message.\n");
  printf("\t-q\tQuiet mode. Suppress output of READER and EMULATOR data (improves timing).\n");
  printf("\n");
  printf("\t[UID]\tUID to emulate, specified as 8 HEX digits (default is DEADBEAF).\n");
}

int main(int argc, char *argv[])
{
  byte_t* pbtTx = NULL;
  size_t szTxBits;
  bool quiet_output = false;

  int arg, i;

  // Get commandline options
  for (arg=1;arg<argc;arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      return 0;
    } else if (0 == strcmp(argv[arg], "-q")) {
      INFO("%s", "Quiet mode.");
      quiet_output = true;
    } else if((arg == argc-1) && (strlen(argv[arg]) == 8)) { // See if UID was specified as HEX string
      byte_t abtTmp[3] = { 0x00,0x00,0x00 };
      printf("[+] Using UID: %s\n",argv[arg]);
      abtUidBcc[4]= 0x00;
      for(i= 0; i < 4; ++i)
      {
        memcpy(abtTmp,argv[arg]+i*2,2);
        abtUidBcc[i]= (byte_t) strtol((char*)abtTmp,NULL,16);
        abtUidBcc[4] ^= abtUidBcc[i];
      }
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      return -1;
    }
  }

  // Try to open the NFC reader
  pnd = nfc_connect(NULL);

  if (pnd == NULL)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }

  printf("\n");
  printf("[+] Connected to NFC reader: %s\n",pnd->acName);
  printf("[+] Try to break out the auto-emulation, this requires a second reader!\n");
  printf("[+] To do this, please send any command after the anti-collision\n");
  printf("[+] For example, send a RATS command or use the \"nfc-anticol\" tool\n");
  if (!nfc_target_init(pnd,abtRecv,&szRecvBits))
  {
    printf("Error: Could not come out of auto-emulation, no command was received\n");
    return 1;
  }
  printf("[+] Received initiator command: ");
  print_hex_bits(abtRecv,szRecvBits);
  printf("[+] Configuring communication\n");
  nfc_configure(pnd,NDO_HANDLE_CRC,false);
  nfc_configure(pnd,NDO_HANDLE_PARITY,true);
  printf("[+] Done, the emulated tag is initialized with UID: %02X%02X%02X%02X\n\n",abtUidBcc[0],abtUidBcc[1],abtUidBcc[2],abtUidBcc[3]);

  while(true)
  {
    // Test if we received a frame
    if (nfc_target_receive_bits(pnd,abtRecv,&szRecvBits,NULL))
    {
      // Prepare the command to send back for the anti-collision request
      switch(szRecvBits)
      {
        case 7: // Request or Wakeup
          pbtTx = abtAtqa;
          szTxBits = 16;
          // New anti-collsion session started
          if (!quiet_output) printf("\n"); 
        break;

        case 16: // Select All
          pbtTx = abtUidBcc;
          szTxBits = 40;
        break;

        case 72: // Select Tag
          pbtTx = abtSak;
          szTxBits = 24;
        break;

        default: // unknown length?
          szTxBits = 0;
        break;
      }

      if(!quiet_output)
      {
        printf("R: ");
        print_hex_bits(abtRecv,szRecvBits);
      }

      // Test if we know how to respond
      if(szTxBits)
      {
        // Send and print the command to the screen
        nfc_target_send_bits(pnd,pbtTx,szTxBits,NULL);
        if(!quiet_output)
        {
          printf("T: ");
          print_hex_bits(pbtTx,szTxBits);
        }
      }
    }
  }

  nfc_disconnect(pnd);
}

