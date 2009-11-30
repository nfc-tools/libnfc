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
 * @file nfc-mfultool.c
 * @brief
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <ctype.h>

#include <nfc.h>

#include "mifareultag.h"
#include "bitutils.h"

static nfc_device_t* pnd;
static nfc_target_info_t nti;
static mifare_param mp;
static mifareul_tag mtDump;
static uint32_t uiBlocks = 0xF;

void print_success_or_failure(bool bFailure, uint32_t* uiBlockCounter)
{
  printf("%c",(bFailure)?'x':'.');
  if (uiBlockCounter)
    *uiBlockCounter += (bFailure)?0:1;
}

bool read_card()
{
  uint32_t page;
  bool bFailure = false;
  uint32_t uiReadBlocks = 0;

  printf("Reading out %d blocks |",uiBlocks+1);

  for (page = 0; page <= uiBlocks;  page += 4)
  {
      // Skip this the first time, bFailure it means nothing (yet)
      if (page != 0)
      {
        print_success_or_failure(bFailure, &uiReadBlocks);
        print_success_or_failure(bFailure, &uiReadBlocks);
        print_success_or_failure(bFailure, &uiReadBlocks);
        print_success_or_failure(bFailure, &uiReadBlocks);
      }

      // Try to read out the data block
      if (nfc_initiator_mifare_cmd(pnd,MC_READ,page,&mp))
      {
        memcpy(mtDump.amb[page / 4].mbd.abtData, mp.mpd.abtData, 16);
      } else {
        bFailure = true;
        break;
      }
  }
  //return bSuccess;
  print_success_or_failure(bFailure, &uiReadBlocks);
  printf("|\n");
  printf("Done, %d of %d blocks read.\n", uiReadBlocks, uiBlocks+1);
  fflush(stdout);

  return (!bFailure);
}

bool write_card()
{
  uint32_t uiBlock = 0;
  int page;
  bool bFailure = false;
  uint32_t uiWriteBlocks = 0;

  for (page = 0x4; page <= 0xF; page++) {
      // Show if the readout went well
      if (bFailure)
      {
        printf("x");
        // When a failure occured we need to redo the anti-collision
        if (!nfc_initiator_select_tag(pnd,NM_ISO14443A_106,NULL,0,&nti))
        {
          printf("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      }
      fflush(stdout);

      // Make sure a earlier write did not fail
      if (!bFailure)
      {
        // For the Mifare Ultralight, this write command can be used
        // in compatibility mode, which only actually writes the first 
        // page (4 bytes). The Ultralight-specific Write command only
        // writes one page at a time.
        uiBlock = page / 4;
        memcpy(mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData + ((page % 4) * 4), 16);
        if (!nfc_initiator_mifare_cmd(pnd, MC_WRITE, page, &mp)) bFailure = true;
      }
  }
  print_success_or_failure(bFailure, &uiWriteBlocks);
  printf("|\n");
  printf("Done, %d of %d blocks written.\n", uiWriteBlocks, uiBlocks+1);
  fflush(stdout);

  return true;
}

int main(int argc, const char* argv[])
{			
  bool bReadAction;
  byte_t* pbtUID;
  FILE* pfDump;

  if (argc < 3)
  {
    printf("\n");
    printf("%s r|w <dump.mfd>\n", argv[0]);
    printf("\n");
    printf("r|w         - Perform read from or write to card\n");
    printf("<dump.mfd>  - MiFare Dump (MFD) used to write (card to MFD) or (MFD to card)\n");
    printf("\n");
    return 1;
  }

  printf("\nChecking arguments and settings\n");

  bReadAction = (tolower(*(argv[1])) == 'r');

  if (bReadAction)
  {
    memset(&mtDump,0x00,sizeof(mtDump));
  } else {
    pfDump = fopen(argv[2],"rb");

    if (pfDump == NULL)
    {
      printf("Could not open dump file: %s\n",argv[2]);
      return 1;
    }

    if (fread(&mtDump,1,sizeof(mtDump),pfDump) != sizeof(mtDump))
    {
      printf("Could not read from dump file: %s\n",argv[2]);
      fclose(pfDump);
      return 1;
    }
    fclose(pfDump);
  }
  printf("Succesful opened the dump file\n");

  // Try to open the NFC reader
  pnd = nfc_connect(NULL);
  if (pnd == NULL)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }

  nfc_initiator_init(pnd);

  // Drop the field for a while
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);

  // Let the reader only try once to find a tag
  nfc_configure(pnd,NDO_INFINITE_SELECT,false);
  nfc_configure(pnd,NDO_HANDLE_CRC,true);
  nfc_configure(pnd,NDO_HANDLE_PARITY,true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,true);

  printf("Connected to NFC reader: %s\n",pnd->acName);

  // Try to find a MIFARE Ultralight tag
  if (!nfc_initiator_select_tag(pnd,NM_ISO14443A_106,NULL,0,&nti))
  {
    printf("Error: no tag was found\n");
    nfc_disconnect(pnd);
    return 1;
  }

  // Test if we are dealing with a MIFARE compatible tag

  if (nti.nai.abtAtqa[1] != 0x44){
      printf("Error: tag is not a MIFARE Ultralight card\n");
    nfc_disconnect(pnd);
      return 1;
  }


  // Get the info from the current tag
  pbtUID = nti.nai.abtUid;
  printf("Found MIFARE Ultralight card with uid: %08x\n", swap_endian32(pbtUID));

  if (bReadAction)
  {
    if (read_card())
    {
      printf("Writing data to file: %s ... ",argv[2]);
      fflush(stdout);
      pfDump = fopen(argv[2],"wb");
      if (pfDump == NULL)
      {
        printf("Could not open file: %s\n",argv[2]);
        return 1;
      }
      if (fwrite(&mtDump,1,sizeof(mtDump),pfDump) != sizeof(mtDump))
      {
        printf("Could not write to file: %s\n",argv[2]);
        return 1;
      }
      fclose(pfDump);
      printf("Done.\n");
    }
  } else {
    write_card();
  }

  nfc_disconnect(pnd);

  return 0;
}
