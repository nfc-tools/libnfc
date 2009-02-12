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

  // Configure the communication channel
  nfc_configure_handle_crc(di,true);
  nfc_configure_handle_parity(di,true);

  printf("\nConnected to NFC reader\n\n");

  uiRecvLen = MAX_FRAME_LEN;
  if (nfc_reader_list_passive(di,MT_ISO14443A_106,null,null,abtRecv,&uiRecvLen))
  {
    // ISO14443A tag info = ( tag_count[1], tag_nr[1], ATQA[2], SAK[1], uid_len[1], UID[uid_len], ats_len[1], ATS[ats_len-1] )
    // ATS is optional
    printf("The following (NFC) ISO14443A tag was found:\n\n");
    printf("%17s","ATQA (SENS_RES): ");
    print_hex(abtRecv+2,2);
    printf("%17s","UID (NFCID1): ");
    print_hex(abtRecv+6,abtRecv[5]);
    printf("%17s","SAK (SEL_RES): ");
    print_hex(abtRecv+4,1);
    if (uiRecvLen > 6+(ui32)abtRecv[5])
    {
      printf("%17s","ATS (ATR): ");
      print_hex(abtRecv+6+abtRecv[5]+1,abtRecv[6+abtRecv[5]]-1);
    }
  } else {
    printf("Error: no tag was found\n");
  }

  // Todo: listing the folllowing tags types 
  // 
  // MT_FELICA_212
  // MT_FELICA_424
  // MT_ISO14443B_106
  // MT_TOPAZ_106

  return 0;
}
