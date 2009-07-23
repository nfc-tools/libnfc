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
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <ctype.h>

#include "libnfc.h"
#include "mifaretag.h"

static dev_info* pdi;
static tag_info ti;
static mifare_param mp;
static mifare_tag mtKeys;
static mifare_tag mtDump;
static bool bUseKeyA;
static uint32_t uiBlocks;

bool is_first_block(uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128) return ((uiBlock)%4 == 0); else return ((uiBlock)%16 == 0);
}

bool is_trailer_block(uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128) return ((uiBlock+1)%4 == 0); else return ((uiBlock+1)%16 == 0);
}

uint32_t get_trailer_block(uint32_t uiFirstBlock)
{
  // Test if we are in the small or big sectors
  if (uiFirstBlock<128) return uiFirstBlock+3; else return uiFirstBlock+15;
}

bool read_card()
{
  int32_t iBlock;
  mifare_cmd mc;
  bool bFailure = false;

  printf("Reading out %d blocks |",uiBlocks+1);

  // Read the card from end to begin
  for (iBlock=uiBlocks; iBlock>=0; iBlock--)
  {
    // Authenticate everytime we reach a trailer block
    if (is_trailer_block(iBlock))
    {
      // Show if the readout went well
      if (bFailure)
      {
        printf("x");
        // When a failure occured we need to redo the anti-collision
        if (!nfc_initiator_select_tag(pdi,IM_ISO14443A_106,NULL,0,&ti))
        {
          printf("!\nError: tag was removed\n");
          return 1;
        }
        bFailure = false;
      } else {
        // Skip this the first time, bFailure it means nothing (yet)
        if (iBlock != uiBlocks)
        {
          printf(".");
        }
      }
      fflush(stdout);

      // Set the authentication information (uid)
      memcpy(mp.mpa.abtUid,ti.tia.abtUid,4);
      
      // Determin if we should use the a or the b key
      if (bUseKeyA)
      {
        mc = MC_AUTH_A;
        memcpy(mp.mpa.abtKey,mtKeys.amb[iBlock].mbt.abtKeyA,6);
      } else {
        mc = MC_AUTH_B;
        memcpy(mp.mpa.abtKey,mtKeys.amb[iBlock].mbt.abtKeyB,6);
      }

      // Try to authenticate for the current sector
      if (!nfc_initiator_mifare_cmd(pdi,MC_AUTH_A,iBlock,&mp))
      {
        printf("!\nError: authentication failed for block %02x\n",iBlock);
        return false;
      }

      // Try to read out the trailer
      if (nfc_initiator_mifare_cmd(pdi,MC_READ,iBlock,&mp))
      {
        // Copy the keys over from our key dump and store the retrieved access bits
        memcpy(mtDump.amb[iBlock].mbt.abtKeyA,mtKeys.amb[iBlock].mbt.abtKeyA,6);
        memcpy(mtDump.amb[iBlock].mbt.abtAccessBits,mp.mpd.abtData+6,4);
        memcpy(mtDump.amb[iBlock].mbt.abtKeyB,mtKeys.amb[iBlock].mbt.abtKeyB,6);
      }
    } else {
      // Make sure a earlier readout did not fail
      if (!bFailure)
      {
        // Try to read out the data block
        if (nfc_initiator_mifare_cmd(pdi,MC_READ,iBlock,&mp))
        {
          memcpy(mtDump.amb[iBlock].mbd.abtData,mp.mpd.abtData,16);
        } else {
          bFailure = true;
        }
      }
    }
  }
  printf("%c|\n",(bFailure)?'x':'.');
  fflush(stdout);

  return true;
}

bool write_card()
{
  uint32_t uiBlock;
  uint32_t uiTrailerBlock;
  bool bFailure = false;
  mifare_cmd mc;

  printf("Writing %d blocks |",uiBlocks+1);

  // Write the card from begin to end;
  for (uiBlock=0; uiBlock<=uiBlocks; uiBlock++)
  {
    // Authenticate everytime we reach the first sector of a new block
    if (is_first_block(uiBlock))
    {
      // Show if the readout went well
      if (bFailure)
      {
        printf("x");
        // When a failure occured we need to redo the anti-collision
        if (!nfc_initiator_select_tag(pdi,IM_ISO14443A_106,NULL,0,&ti))
        {
          printf("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      } else {
        // Skip this the first time, bFailure it means nothing (yet)
        if (uiBlock != 0)
        {
          printf(".");
        }
      }
      fflush(stdout);

      // Locate the trailer (with the keys) used for this sector
      uiTrailerBlock = get_trailer_block(uiBlock);

      // Set the authentication information (uid)
      memcpy(mp.mpa.abtUid,ti.tia.abtUid,4);
      
      // Determin if we should use the a or the b key
      if (bUseKeyA)
      {
        mc = MC_AUTH_A;
        memcpy(mp.mpa.abtKey,mtKeys.amb[uiTrailerBlock].mbt.abtKeyA,6);
      } else {
        mc = MC_AUTH_B;
        memcpy(mp.mpa.abtKey,mtKeys.amb[uiTrailerBlock].mbt.abtKeyB,6);
      }

      // Try to authenticate for the current sector
      if (!nfc_initiator_mifare_cmd(pdi,mc,uiBlock,&mp))
      { 
        printf("!\nError: authentication failed for block %02x\n",uiBlock);
        return false;
      }
    }
    
    if (is_trailer_block(uiBlock))
    {
      // Copy the keys over from our key dump and store the retrieved access bits
      memcpy(mp.mpd.abtData,mtDump.amb[uiBlock].mbt.abtKeyA,6);
      memcpy(mp.mpd.abtData+6,mtDump.amb[uiBlock].mbt.abtAccessBits,4);
      memcpy(mp.mpd.abtData+10,mtDump.amb[uiBlock].mbt.abtKeyB,6);

      // Try to write the trailer
      nfc_initiator_mifare_cmd(pdi,MC_WRITE,uiBlock,&mp);
    
    } else {

      // The first block 0x00 is read only, skip this
      if (uiBlock == 0) continue;

      // Make sure a earlier write did not fail
      if (!bFailure)
      {
        // Try to write the data block
        memcpy(mp.mpd.abtData,mtDump.amb[uiBlock].mbd.abtData,16);
        if (!nfc_initiator_mifare_cmd(pdi,MC_WRITE,uiBlock,&mp)) bFailure = true;
      }
    }
  }
  printf("%c|\n",(bFailure)?'x':'.');
  fflush(stdout);
  
  return true;
}

int main(int argc, const char* argv[])
{			
  bool b4K;
  bool bReadAction;
  byte_t* pbtUID;
  FILE* pfKeys;
  FILE* pfDump;

  if (argc < 5)
  {
    printf("\n");
    printf("mftool <r|w> <a|b> <keys.mfd> <dump.mfd>\n");
    printf("\n");
    printf("<r|w>       - Perform (read from) or (write to) card\n");
    printf("<a|b>       - Use A or B keys to for action\n");
    printf("<keys.mfd>  - Mifare-dump that contain the keys\n");
    printf("<dump.mfd>  - Used to write (card to file) or (file to card)\n");
    printf("\n");
    return 1;
  }

  printf("\nChecking arguments and settings\n");

  bReadAction = (tolower(*(argv[1])) == 'r');
  bUseKeyA = (tolower(*(argv[2])) == 'a');

  pfKeys = fopen(argv[3],"rb");
  if (pfKeys == NULL)
  {
    printf("Could not open file: %s\n",argv[3]);
    return 1;
  }
  if (fread(&mtKeys,1,sizeof(mtKeys),pfKeys) != sizeof(mtKeys))
  {
    printf("Could not read from keys file: %s\n",argv[3]);
    fclose(pfKeys);
    return 1;
  }
  fclose(pfKeys);

  if (bReadAction)
  {
    memset(&mtDump,0x00,sizeof(mtDump));
  } else {
    pfDump = fopen(argv[4],"rb");

    if (pfDump == NULL)
    {
      printf("Could not open dump file: %s\n",argv[4]);
      return 1;
    }

    if (fread(&mtDump,1,sizeof(mtDump),pfDump) != sizeof(mtDump))
    {
      printf("Could not read from dump file: %s\n",argv[4]);
      fclose(pfDump);
      return 1;
    }
    fclose(pfDump);
  }
  printf("Succesful opened MIFARE the required files\n");

  // Try to open the NFC reader
  pdi = nfc_connect();
  if (pdi == INVALID_DEVICE_INFO)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }

  nfc_initiator_init(pdi);

  // Drop the field for a while
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,false);
  
  // Let the reader only try once to find a tag
  nfc_configure(pdi,DCO_INFINITE_SELECT,false);
  nfc_configure(pdi,DCO_HANDLE_CRC,true);
  nfc_configure(pdi,DCO_HANDLE_PARITY,true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,true);

  printf("Connected to NFC reader: %s\n",pdi->acName);

  // Try to find a MIFARE Classic tag
  if (!nfc_initiator_select_tag(pdi,IM_ISO14443A_106,NULL,0,&ti))
  {
    printf("Error: no tag was found\n");
    nfc_disconnect(pdi);
    return 1;
  }

  // Test if we are dealing with a MIFARE compatible tag
  if ((ti.tia.btSak & 0x08) == 0)
  {
    printf("Error: tag is not a MIFARE Classic card\n");
    nfc_disconnect(pdi);
    return 1;
  }

  // Get the info from the key dump
  b4K = (mtKeys.amb[0].mbm.abtATQA[1] == 0x02);
  pbtUID = mtKeys.amb[0].mbm.abtUID;

  // Compare if key dump UID is the same as the current tag UID
  if (memcmp(ti.tia.abtUid,pbtUID,4) != 0)
  {
    printf("Expected MIFARE Classic %cK card with uid: %08x\n",b4K?'4':'1',swap_endian32(pbtUID));
  }

  // Get the info from the current tag
  pbtUID = ti.tia.abtUid;
  b4K = (ti.tia.abtAtqa[1] == 0x02);
  printf("Found MIFARE Classic %cK card with uid: %08x\n",b4K?'4':'1',swap_endian32(pbtUID));

  uiBlocks = (b4K)?0xff:0x3f;

  if (bReadAction)
  {
    if (read_card())
    {
      printf("Writing data to file: %s\n",argv[4]);
      fflush(stdout);
      pfDump = fopen(argv[4],"wb");
      if (pfKeys == NULL)
      {
        printf("Could not open file: %s\n",argv[4]);
        return 1;
      }
      if (fwrite(&mtDump,1,sizeof(mtDump),pfDump) != sizeof(mtDump))
      {
        printf("Could not write to file: %s\n",argv[4]);
        return 1;
      }
      fclose(pfDump);
      printf("Done, all bytes dumped to file!\n");
    }
  } else {
    if (write_card())
    {
      printf("Done, all data is written to the card!\n");
    }
  }

  nfc_disconnect(pdi);

  return 0;
}
