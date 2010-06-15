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
 * @file nfc-sam.c
 * @brief Configure the reader to comunicate with a SAM (Secure Access Module).
 */
 
#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

#define VIRTUAL_CARD_MODE 2
#define WIRED_CARD_MODE 3
#define DUAL_CARD_MODE 4

int main(int argc, const char* argv[])
{
  nfc_device_t* pnd;
  
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  
  byte_t abtSAMConfig[] = { 0xD4,0x14,0x00,0x00 };
  
  nfc_target_info_t nti;

  // Display libnfc version
  const char* acLibnfcVersion = nfc_version();
  printf("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  // Connect using the first available NFC device
  pnd = nfc_connect(NULL);

  if (pnd == NULL) {
      printf("Unable to connect to NFC device.");
      return EXIT_FAILURE;
  }

  // Set connected NFC device to initiator mode
  nfc_initiator_init(pnd);

  // Drop the field for a while
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);

  // Configure the CRC and Parity settings
  nfc_configure(pnd,NDO_HANDLE_CRC,false);
  nfc_configure(pnd,NDO_HANDLE_PARITY,true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,true);

  printf("Connected to NFC reader: %s\n",pnd->acName);
  
  // Print the example's menu.
  printf("\nSelect the comunication mode:\n");
  printf("[1] Virtual card mode.\n");
  printf("[2] Wired card mode.\n");
  printf("[3] Dual card mode.\n");
  printf(">> ");
  
  // Take user's choice.
  char input = getchar();
  int mode = input-'0'+1;
  printf("\n");
  if (mode <= 1 || mode >= 5)
    return 1;
    
  abtSAMConfig[2] = mode;
  
  // Connect with the SAM.
  pnd->pdc->transceive(pnd->nds,abtSAMConfig,sizeof(abtSAMConfig),abtRx,&szRxLen);
  
  switch (mode)
  {
    case VIRTUAL_CARD_MODE:
    {
      printf("Now the SAM is readable from an external reader.\n");
      // TODO.
    }
    break;
        
    case WIRED_CARD_MODE:
    {        
      // Read the SAM's ATS (ATR).
      if (nfc_initiator_select_tag(pnd,NM_ISO14443A_106,NULL,0,&nti))
      {
        printf("The following (NFC) ISO14443A tag was found:\n\n");
        print_nfc_iso14443a_info (nti.nai);
      }
    }
    break;
    
    case DUAL_CARD_MODE:
    {
      // TODO.
    }
    break;
  }

  // Disconnect from NFC device
  nfc_disconnect(pnd);
  
  return EXIT_SUCCESS;
}
