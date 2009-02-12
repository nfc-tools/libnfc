/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libnfc.h"

static byte abtRecv[MAX_FRAME_LEN];
static ui32 uiRecvLen;
static dev_id di;

// ISO14443A Anti-Collision response
byte abtAtqa      [2] = { 0x04,0x00 };
byte abtUidBcc    [5] = { 0xDE,0xAD,0xBE,0xAF,0x62 };
byte abtSak       [9] = { 0x08,0xb6,0xdd };

int main(int argc, const char* argv[])
{			
  byte* pbtTx;
  ui32 uiTxLen;
  
  // Try to open the NFC reader
  di = acr122_connect(0);
  
  if (di == INVALID_DEVICE_ID)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }

  printf("\n");
  printf("[+] Connected to NFC target\n");
  acr122_led_red(di,true);
  printf("[+] Identified simulated tag by setting the red light\n");
  printf("[+] First we have to come out auto-simulation\n");
  printf("[+] To do this, please send any command after the\n");
  printf("[+] anti-collision, for example, the RATS command\n\n");
  nfc_target_init(di,abtRecv,&uiRecvLen);
  printf("[+] Initiator command: ");
  print_hex(abtRecv,uiRecvLen);
  printf("[+] Configuring communication");
  nfc_configure_accept_invalid_frames(di,true);
  nfc_configure_handle_crc(di,false);
  printf("[+] Done, the simulated tag is initialized \n");


  while(true)
  {
    // Test if we received a frame
    if (nfc_target_receive_bytes(di,abtRecv,&uiRecvLen))
    {
      // Prepare the command to send back for the anti-collision request
      switch(uiRecvLen)
      {
        case 1: // Request or Wakeup
          pbtTx = abtAtqa;
          uiTxLen = 2;
          // New anti-collsion session started
          printf("\n"); 
        break;

        case 2: // Select All
          pbtTx = abtUidBcc;
          uiTxLen = 5;
        break;

        case 9: // Select Tag
          pbtTx = abtSak;
          uiTxLen = 3;
        break;

        default: // unknown length?
          uiTxLen = 0;
        break;
      }

      printf("R: ");
      print_hex(abtRecv,uiRecvLen);

      // Test if we know how to respond
      if(uiTxLen)
      {
        // Send and print the command to the screen
        nfc_target_send_bytes(di,pbtTx,uiTxLen);
        printf("T: ");
        print_hex(pbtTx,uiTxLen);
      }
    }
  }
}
