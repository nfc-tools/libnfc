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
#include <time.h>

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

#define NORMAL_MODE 1
#define VIRTUAL_CARD_MODE 2
#define WIRED_CARD_MODE 3
#define DUAL_CARD_MODE 4

bool sam_connection(nfc_device_t* pnd, int mode)
{
  byte_t pncmd_sam_config[] = { 0xD4,0x14,0x00,0x00 };
  
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  
  // Only the VIRTUAL_CARD_MODE requires 4 bytes.
  int size = sizeof(pncmd_sam_config)-((mode == VIRTUAL_CARD_MODE) ? 0 : 1);
  pncmd_sam_config[2] = mode;
  
  return pnd->pdc->transceive(pnd->nds,pncmd_sam_config,size,abtRx,&szRxLen);
}

void wait_one_minute()
{
  int secs = 0;
  
  printf("|");
  fflush(stdout);
  
  while (secs < 60)
  {
    sleep(1);
    secs += 1;
    printf(".");
    fflush(stdout);
  }
  
  printf("|\n");
}

int main(int argc, const char* argv[])
{
  nfc_device_t* pnd;

  // Display libnfc version
  const char* acLibnfcVersion = nfc_version();
  printf("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  // Connect using the first available NFC device
  pnd = nfc_connect(NULL);

  if (pnd == NULL) {
      ERR("%s", "Unable to connect to NFC device.");
      return EXIT_FAILURE;
  }

  printf("Connected to NFC reader: %s\n",pnd->acName);
  
  // Print the example's menu
  printf("\nSelect the comunication mode:\n");
  printf("[1] Virtual card mode.\n");
  printf("[2] Wired card mode.\n");
  printf("[3] Dual card mode.\n");
  printf(">> ");
  
  // Take user's choice
  char input = getchar();
  int mode = input-'0'+1;
  printf("\n");
  if (mode < VIRTUAL_CARD_MODE || mode > DUAL_CARD_MODE) {
      ERR("%s", "Invalid selection.");
      return EXIT_FAILURE;
  }
  
  // Connect with the SAM
  sam_connection(pnd, mode);
  
  switch (mode)
  {
    case VIRTUAL_CARD_MODE:
    {
      // FIXME: after the loop the reader doesn't respond to host commands...
      printf("Now the SAM is readable for 1 minute from an external reader.\n");
      wait_one_minute();
    }
    break;
        
    case WIRED_CARD_MODE:
    {
      nfc_target_info_t nti;
      
      // Set connected NFC device to initiator mode
      nfc_initiator_init(pnd);

      // Drop the field for a while
      nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);

      // Let the reader only try once to find a tag
      nfc_configure(pnd,NDO_INFINITE_SELECT,false);

      // Configure the CRC and Parity settings
      nfc_configure(pnd,NDO_HANDLE_CRC,true);
      nfc_configure(pnd,NDO_HANDLE_PARITY,true);

      // Enable field so more power consuming cards can power themselves up
      nfc_configure(pnd,NDO_ACTIVATE_FIELD,true);
      
      // Read the SAM's info
      if (!nfc_initiator_select_tag(pnd,NM_ISO14443A_106,NULL,0,&nti)) {
        ERR("%s", "Reading of SAM info failed.");
        return EXIT_FAILURE;
      }
      
      printf("The following ISO14443A tag (SAM) was found:\n\n");
      print_nfc_iso14443a_info (nti.nai);
    }
    break;
    
    case DUAL_CARD_MODE:
    {
      byte_t abtRx[MAX_FRAME_LEN];
      size_t szRxLen;
      
      // FIXME: it does not work as expected...
      if (!nfc_target_init(pnd,abtRx,&szRxLen))
        return EXIT_FAILURE;
      
      printf("Now both the NFC reader and SAM are readable for 1 minute from an external reader.\n");
      wait_one_minute();
    }
    break;
  }
  
  // Disconnect from the SAM.
  sam_connection(pnd, NORMAL_MODE);

  // Disconnect from NFC device
  nfc_disconnect(pnd);
  
  return EXIT_SUCCESS;
}
