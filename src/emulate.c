/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#include "libnfc.h"

static byte_t abtRecv[MAX_FRAME_LEN];
static uint32_t uiRecvBits;
static dev_info* pdi;

// ISO14443A Anti-Collision response
byte_t abtAtqa      [2] = { 0x04,0x00 };
byte_t abtUidBcc    [5] = { 0xDE,0xAD,0xBE,0xAF,0x62 };
byte_t abtSak       [9] = { 0x08,0xb6,0xdd };
byte_t Tmp          [3] = { 0x00,0x00,0x00 };

int main(int argc, char *argv[])
{                       
  byte_t* pbtTx = NULL;
  uint32_t uiTxBits;
  int i, quiet= 0;

  // Get commandline options
  while ((i= getopt(argc, argv, "hq")) != -1)
    switch (i)
    {
    case 'q':
      quiet= 1;
      break;
    case 'h':
    default:
      printf("\n\tusage:\n");
      printf("\t\tnfc-emulate [OPTIONS] [UID]\n\n");
      printf("\toptions:\n");
      printf("\t\t-h\tHelp. Print this message.\n");
      printf("\t\t-q\tQuiet mode. Suppress output of READER and EMULATOR data (improves timing).\n");
      printf("\n");
      printf("\targs:\n");
      printf("\t\t[UID]\tThe UID to emulate, specified as 8 HEX digits. Default is DEADBEAF.\n");
      printf("\n");
      return -1;
    }

  // See if UID was specified as HEX string
  if(argc > 1 && strlen(argv[optind]) == 8)
  {
    printf("[+] Using UID: %s\n",argv[optind]);
    abtUidBcc[4]= 0x00;
    for(i= 0; i < 4; ++i)
    { 
      memcpy(Tmp,argv[optind]+i*2,2);
      abtUidBcc[i]= (byte_t) strtol(Tmp,NULL,16);
      abtUidBcc[4] ^= abtUidBcc[i];
    }
  }

  // Try to open the NFC reader
  pdi = nfc_connect();
  
  if (pdi == INVALID_DEVICE_INFO)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }

  printf("\n");
  printf("[+] Connected to NFC reader: %s\n",pdi->acName);
  printf("[+] Try to break out the auto-emulation, this requires a second reader!\n");
  printf("[+] To do this, please send any command after the anti-collision\n");
  printf("[+] For example, send a RATS command or use the \"anticol\" tool\n");
  if (!nfc_target_init(pdi,abtRecv,&uiRecvBits))
  {
    printf("Error: Could not come out of auto-emulation, no command was received\n");
    return 1;
  }
  printf("[+] Received initiator command: ");
  print_hex_bits(abtRecv,uiRecvBits);
  printf("[+] Configuring communication\n");
  nfc_configure(pdi,DCO_HANDLE_CRC,false);
  nfc_configure(pdi,DCO_HANDLE_PARITY,true);
  printf("[+] Done, the emulated tag is initialized with UID: %02X%02X%02X%02X\n\n",abtUidBcc[0],abtUidBcc[1],abtUidBcc[2],abtUidBcc[3]);

  while(true)
  {
    // Test if we received a frame
    if (nfc_target_receive_bits(pdi,abtRecv,&uiRecvBits,NULL))
    {
      // Prepare the command to send back for the anti-collision request
      switch(uiRecvBits)
      {
        case 7: // Request or Wakeup
          pbtTx = abtAtqa;
          uiTxBits = 16;
          // New anti-collsion session started
          if (!quiet) printf("\n"); 
        break;

        case 16: // Select All
          pbtTx = abtUidBcc;
          uiTxBits = 40;
        break;

        case 72: // Select Tag
          pbtTx = abtSak;
          uiTxBits = 24;
        break;

        default: // unknown length?
          uiTxBits = 0;
        break;
      }

      if(!quiet)
      {
        printf("R: ");
        print_hex_bits(abtRecv,uiRecvBits);
      }

      // Test if we know how to respond
      if(uiTxBits)
      {
        // Send and print the command to the screen
        nfc_target_send_bits(pdi,pbtTx,uiTxBits,NULL);
        if(!quiet)
        {
          printf("T: ");
          print_hex_bits(pbtTx,uiTxBits);
        }
      }
    }
  }

  nfc_disconnect(pdi);
}

