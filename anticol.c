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
static byte abtUid[10];
static ui32 uiUidLen = 4;
static dev_id di;

// ISO14443A Anti-Collision Commands
byte abtWupa      [1] = { 0x52 };
byte abtSelectAll [2] = { 0x93,0x20 };
byte abtSelectTag [9] = { 0x93,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
byte abtRats      [4] = { 0xe0,0x50,0xbc,0xa5 };
byte abtHalt      [4] = { 0x50,0x00,0x57,0xcd };

bool transmit_7bits(const byte btTx)
{
  bool bResult;
  printf("R: %02x\n",btTx); 
  uiRecvLen = MAX_FRAME_LEN;
  bResult = nfc_reader_transceive_7bits(di,btTx,abtRecv,&uiRecvLen);
  if (bResult)
  {
    printf("T: "); 
    print_hex(abtRecv,uiRecvLen);
  }
  return bResult;
}

bool transmit_bytes(const byte* pbtTx, const ui32 uiTxLen)
{
  bool bResult;
  printf("R: ");
  print_hex(pbtTx,uiTxLen);
  uiRecvLen = MAX_FRAME_LEN;
  bResult = nfc_reader_transceive_bytes(di,pbtTx,uiTxLen,abtRecv,&uiRecvLen);
  if (bResult)
  {
    printf("T: ");
    print_hex(abtRecv,uiRecvLen);
  }
  return bResult;
}

int main(int argc, const char* argv[])
{			
  // Try to open the NFC reader
  di = acr122_connect(0);
  
  if (di == INVALID_DEVICE_ID)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }
  nfc_reader_init(di);

  // Let the reader only try once to find a tag
  nfc_configure_list_passive_infinite(di,false);

  // Drop the field so the tag will be reset
  nfc_configure_field(di,false);

  // Configure the communication channel, we use our own CRC
  nfc_configure_handle_crc(di,false);
  nfc_configure_handle_parity(di,true);

  // Enable the field so the more power consuming tags will respond
  nfc_configure_field(di,true);

  printf("\nConnected to NFC reader\n\n");

  if (!transmit_7bits(*abtWupa))
  {
    printf("Error: No tag available\n");
    return 1;
  }
  // Anti-collision
  transmit_bytes(abtSelectAll,2);
  
  // Save the UID
  memcpy(abtUid,abtRecv,4);
  memcpy(abtSelectTag+2,abtRecv,5);
  append_iso14443a_crc(abtSelectTag,7);
  transmit_bytes(abtSelectTag,9);

  if (abtUid[0] == 0x88)
  {
    abtSelectAll[0] = 0x95;
    abtSelectTag[0] = 0x95;

    // Anti-collision
    transmit_bytes(abtSelectAll,2);
    
    // Save the UID
    memcpy(abtUid+4,abtRecv,4);
    memcpy(abtSelectTag+2,abtRecv,5);
    append_iso14443a_crc(abtSelectTag,7);
    transmit_bytes(abtSelectTag,9);
    uiUidLen = 7;
  }
  
  // Request ATS, this only applies to tags that support ISO 14443A-4
  if (abtRecv[0] & 0x20)
  {
    transmit_bytes(abtRats,4);
  }

  // Done, halt the tag now
  transmit_bytes(abtHalt,4);

  printf("\nFound tag with UID: ");
  if (uiUidLen == 4)
  {
    printf("%08x\n",swap_endian32(abtUid));
  } else {
    printf("%014llx\n",swap_endian64(abtUid)&0x00ffffffffffffffull);
  }
  return 0;
}
