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

static byte abtReaderRecv[MAX_FRAME_LEN];
static byte abtReaderRecvPar[MAX_FRAME_LEN];
static ui32 uiReaderRecvBits;
static byte abtTagRecv[MAX_FRAME_LEN];
static byte abtTagRecvPar[MAX_FRAME_LEN];
static ui32 uiTagRecvBits;
static dev_id diReader;
static dev_id diTag;

int main(int argc, const char* argv[])
{			
  // Try to open the NFC reader
  diReader = acr122_connect(0);
  if (diReader == INVALID_DEVICE_ID)
  {
    printf("Error connecting first NFC reader\n");
    return 1;
  }

  diTag = acr122_connect(1);
  if (diTag == INVALID_DEVICE_ID)
  {
    printf("Error connecting second NFC reader\n");
    return 1;
  }

  printf("\n");
  printf("[+] Connected to the both NFC readers\n");
  acr122_led_red(diTag,true);
  printf("[+] Identified simulated tag by setting the red light\n");
  printf("[+] Place both readers on top of each other\n");
  printf("[+] Please run 'anticol' tool in a different shell\n");
  nfc_target_init(diTag,abtReaderRecv,&uiReaderRecvBits);
  nfc_configure_handle_crc(diTag,false);
  nfc_configure_handle_parity(diTag,false);
  nfc_configure_accept_invalid_frames(diTag,true);
  printf("[+] Thank you, the simulated tag is initialized\n");
  
  printf("[+] Attaching to first NFC reader \n");
  // Retry until it becomes ready
  while (!nfc_reader_init(diReader))
  {
    acr122_disconnect(diReader);
    printf("error\n");
    diReader = acr122_connect(0);
  }
  nfc_configure_handle_crc(diReader,false);
  nfc_configure_handle_parity(diReader,false);
  nfc_configure_accept_invalid_frames(diReader,true);
  printf("[+] Done, relaying frames now!\n\n");

  while(true)
  {
    // Test if we received a frame from the reader
    if (nfc_target_receive_bits(diTag,abtReaderRecv,&uiReaderRecvBits,abtReaderRecvPar))
    {
      // Drop down the field before sending a REQA command and start a new session
      if (uiReaderRecvBits == 7 && abtReaderRecv[0] == 0x26)
      {
        // Drop down field for a very short time (tag will reboot)
        nfc_configure_field(diReader,false);
        printf("\n");
        nfc_configure_field(diReader,true);
      }

      // Print the reader frame to the screen
      printf("R: ");
      print_hex_par(abtReaderRecv,uiReaderRecvBits,abtReaderRecvPar);

      // Forward the frame to the original tag
      if (nfc_reader_transceive_bits(diReader,abtReaderRecv,uiReaderRecvBits,abtReaderRecvPar,abtTagRecv,&uiTagRecvBits,abtTagRecvPar))
      {
        // Redirect the answer back to the reader
        nfc_target_send_bits(diTag,abtTagRecv,uiTagRecvBits,abtTagRecvPar);
        
        // Print the tag frame to the screen
        printf("T: ");
        print_hex_par(abtTagRecv,uiTagRecvBits,abtTagRecvPar);
      }
    }
  }
}
