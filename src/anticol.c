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
#include <stdbool.h>

#include <string.h>

#include "libnfc.h"

#define SAK_FLAG_ATS_SUPPORTED 0x20

static byte_t abtRx[MAX_FRAME_LEN];
static uint32_t uiRxBits;
static uint32_t uiRxLen;
static byte_t abtUid[10];
static uint32_t uiUidLen = 4;
static dev_info* pdi;

// ISO14443A Anti-Collision Commands
byte_t abtReqa      [1] = { 0x26 };
byte_t abtSelectAll [2] = { 0x93,0x20 };
byte_t abtSelectTag [9] = { 0x93,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
byte_t abtRats      [4] = { 0xe0,0x50,0xbc,0xa5 };
byte_t abtHalt      [4] = { 0x50,0x00,0x57,0xcd };

bool transmit_bits(const byte_t* pbtTx, const uint32_t uiTxBits)
{
  // Show transmitted command
  printf("R: "); print_hex_bits(pbtTx,uiTxBits);

  // Transmit the bit frame command, we don't use the arbitrary parity feature
  if (!nfc_initiator_transceive_bits(pdi,pbtTx,uiTxBits,NULL,abtRx,&uiRxBits,NULL)) return false;

  // Show received answer
  printf("T: "); print_hex_bits(abtRx,uiRxBits);

  // Succesful transfer
  return true;
}


bool transmit_bytes(const byte_t* pbtTx, const uint32_t uiTxLen)
{
  // Show transmitted command
  printf("R: "); print_hex(pbtTx,uiTxLen);

  // Transmit the command bytes
  if (!nfc_initiator_transceive_bytes(pdi,pbtTx,uiTxLen,abtRx,&uiRxLen)) return false;

  // Show received answer
  printf("T: "); print_hex(abtRx,uiRxLen);

  // Succesful transfer
  return true;
}

int main(int argc, const char* argv[])
{
  // Try to open the NFC reader
  pdi = nfc_connect();
  
  if (!pdi)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }
  nfc_initiator_init(pdi);

  // Drop the field for a while
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,false);

  // Configure the CRC and Parity settings
  nfc_configure(pdi,DCO_HANDLE_CRC,false);
  nfc_configure(pdi,DCO_HANDLE_PARITY,true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,true);
  
  printf("\nConnected to NFC reader: %s\n\n",pdi->acName);

  // Send the 7 bits request command specified in ISO 14443A (0x26)
  if (!transmit_bits(abtReqa,7))
  {
    printf("Error: No tag available\n");
    nfc_disconnect(pdi);
    return 1;
  }

  // Anti-collision
  transmit_bytes(abtSelectAll,2);
  
  // Save the UID
  memcpy(abtUid,abtRx,4);
  memcpy(abtSelectTag+2,abtRx,5);
  append_iso14443a_crc(abtSelectTag,7);
  transmit_bytes(abtSelectTag,9);

  // Test if we are dealing with a 4 bytes uid
  if (abtUid[0]!= 0x88)
  {
    uiUidLen = 4;
  } else {
    // We have to do the anti-collision for cascade level 2
    abtSelectAll[0] = 0x95;
    abtSelectTag[0] = 0x95;

    // Anti-collision
    transmit_bytes(abtSelectAll,2);
    
    // Save the UID
    memcpy(abtUid+4,abtRx,4);
    memcpy(abtSelectTag+2,abtRx,5);
    append_iso14443a_crc(abtSelectTag,7);
    transmit_bytes(abtSelectTag,9);
    uiUidLen = 7;
  }
  
  // Request ATS, this only applies to tags that support ISO 14443A-4
  if (abtRx[0] & SAK_FLAG_ATS_SUPPORTED) transmit_bytes(abtRats,4);

  // Done, halt the tag now
  transmit_bytes(abtHalt,4);
  
  printf("\nFound tag with UID: ");
  if (uiUidLen == 4)
  {
    printf("%08x\n",swap_endian32(abtUid));
  } else {
    printf("%014llx\n",swap_endian64(abtUid)&0x00ffffffffffffffull);
  }

  nfc_disconnect(pdi);
  return 0;
}
