/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romuald Conty, Romain Tartière
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer. 
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-mfclassic.c
 * @brief MIFARE Classic manipulation example
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <ctype.h>

#include <nfc/nfc.h>

#include "mifare.h"
#include "nfc-utils.h"

static nfc_device_t *pnd;
static nfc_target_t nt;
static mifare_param mp;
static mifare_classic_tag mtKeys;
static mifare_classic_tag mtDump;
static bool bUseKeyA;
static bool bUseKeyFile;
static uint8_t uiBlocks;
static byte_t keys[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
  0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd,
  0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a,
  0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xab, 0xcd, 0xef, 0x12, 0x34, 0x56
};

static const nfc_modulation_t nmMifare = {
  .nmt = NMT_ISO14443A,
  .nbr = NBR_106,
};

static size_t num_keys = sizeof (keys) / 6;

static void
print_success_or_failure (bool bFailure, uint32_t * uiBlockCounter)
{
  printf ("%c", (bFailure) ? 'x' : '.');
  if (uiBlockCounter && !bFailure)
    *uiBlockCounter += (*uiBlockCounter < 128) ? 4 : 16;
}

static  bool
is_first_block (uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock) % 4 == 0);
  else
    return ((uiBlock) % 16 == 0);
}

static  bool
is_trailer_block (uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

static  uint32_t
get_trailer_block (uint32_t uiFirstBlock)
{
  // Test if we are in the small or big sectors
  uint32_t trailer_block = 0;
  if (uiFirstBlock < 128) {
    trailer_block = uiFirstBlock + (3 - (uiFirstBlock % 4));
  } else {
    trailer_block = uiFirstBlock + (15 - (uiFirstBlock % 16));
  }
  return trailer_block;
}

static  bool
authenticate (uint32_t uiBlock)
{
  mifare_cmd mc;
  uint32_t uiTrailerBlock;
  size_t  key_index;

  // Key file authentication.
  if (bUseKeyFile) {
    // Set the authentication information (uid)
    memcpy (mp.mpa.abtUid, nt.nti.nai.abtUid, 4);

    // Locate the trailer (with the keys) used for this sector
    uiTrailerBlock = get_trailer_block (uiBlock);

    // Determin if we should use the a or the b key
    if (bUseKeyA) {
      mc = MC_AUTH_A;
      memcpy (mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyA, 6);
    } else {
      mc = MC_AUTH_B;
      memcpy (mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyB, 6);
    }

    // Try to authenticate for the current sector
    if (nfc_initiator_mifare_cmd (pnd, mc, uiBlock, &mp))
      return true;
  }
  // Auto authentication.
  else {
    // Determin if we should use the a or the b key
    mc = (bUseKeyA) ? MC_AUTH_A : MC_AUTH_B;

    // Set the authentication information (uid)
    memcpy (mp.mpa.abtUid, nt.nti.nai.abtUid, 4);

    for (key_index = 0; key_index < num_keys; key_index++) {
      memcpy (mp.mpa.abtKey, keys + (key_index * 6), 6);
      if (nfc_initiator_mifare_cmd (pnd, mc, uiBlock, &mp)) {
        if (bUseKeyA)
          memcpy (mtKeys.amb[uiBlock].mbt.abtKeyA, &mp.mpa.abtKey, 6);
        else
          memcpy (mtKeys.amb[uiBlock].mbt.abtKeyB, &mp.mpa.abtKey, 6);

        return true;
      }

      nfc_initiator_select_passive_target (pnd, nmMifare, mp.mpa.abtUid, 4, NULL);
    }
  }

  return false;
}

static  bool
read_card (void)
{
  int32_t iBlock;
  bool    bFailure = false;
  uint32_t uiReadBlocks = 0;

  printf ("Reading out %d blocks |", uiBlocks + 1);

  // Read the card from end to begin
  for (iBlock = uiBlocks; iBlock >= 0; iBlock--) {
    // Authenticate everytime we reach a trailer block
    if (is_trailer_block (iBlock)) {
      // Skip this the first time, bFailure it means nothing (yet)
      if (iBlock != uiBlocks)
        print_success_or_failure (bFailure, &uiReadBlocks);

      // Show if the readout went well
      if (bFailure) {
        // When a failure occured we need to redo the anti-collision
        if (!nfc_initiator_select_passive_target (pnd, nmMifare, NULL, 0, &nt)) {
          printf ("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      }

      fflush (stdout);

      // Try to authenticate for the current sector
      if (!authenticate (iBlock)) {
        printf ("!\nError: authentication failed for block 0x%02x\n", iBlock);
        return false;
      }
      // Try to read out the trailer
      if (nfc_initiator_mifare_cmd (pnd, MC_READ, iBlock, &mp)) {
        // Copy the keys over from our key dump and store the retrieved access bits
        memcpy (mtDump.amb[iBlock].mbt.abtKeyA, mtKeys.amb[iBlock].mbt.abtKeyA, 6);
        memcpy (mtDump.amb[iBlock].mbt.abtAccessBits, mp.mpd.abtData + 6, 4);
        memcpy (mtDump.amb[iBlock].mbt.abtKeyB, mtKeys.amb[iBlock].mbt.abtKeyB, 6);
      } else {
        printf ("!\nError: unable to read trailer block 0x%02x\n", iBlock);
      }
    } else {
      // Make sure a earlier readout did not fail
      if (!bFailure) {
        // Try to read out the data block
        if (nfc_initiator_mifare_cmd (pnd, MC_READ, iBlock, &mp)) {
          memcpy (mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, 16);
        } else {
          bFailure = true;
          printf ("!\nError: unable to read block 0x%02x\n", iBlock);
          return false;
        }
      }
    }
  }
  print_success_or_failure (bFailure, &uiReadBlocks);
  printf ("|\n");
  printf ("Done, %d of %d blocks read.\n", uiReadBlocks, uiBlocks + 1);
  fflush (stdout);

  return true;
}

static  bool
write_card (void)
{
  uint32_t uiBlock;
  bool    bFailure = false;
  uint32_t uiWriteBlocks = 0;

  printf ("Writing %d blocks |", uiBlocks + 1);

  // Write the card from begin to end;
  for (uiBlock = 0; uiBlock <= uiBlocks; uiBlock++) {
    // Authenticate everytime we reach the first sector of a new block
    if (is_first_block (uiBlock)) {
      // Skip this the first time, bFailure it means nothing (yet)
      if (uiBlock != 0)
        print_success_or_failure (bFailure, &uiWriteBlocks);

      // Show if the readout went well
      if (bFailure) {
        // When a failure occured we need to redo the anti-collision
        if (!nfc_initiator_select_passive_target (pnd, nmMifare, NULL, 0, &nt)) {
          printf ("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      }

      fflush (stdout);

      // Try to authenticate for the current sector
      if (!authenticate (uiBlock)) {
        printf ("!\nError: authentication failed for block %02x\n", uiBlock);
        return false;
      }
    }

    if (is_trailer_block (uiBlock)) {
      // Copy the keys over from our key dump and store the retrieved access bits
      memcpy (mp.mpd.abtData, mtDump.amb[uiBlock].mbt.abtKeyA, 6);
      memcpy (mp.mpd.abtData + 6, mtDump.amb[uiBlock].mbt.abtAccessBits, 4);
      memcpy (mp.mpd.abtData + 10, mtDump.amb[uiBlock].mbt.abtKeyB, 6);

      // Try to write the trailer
      if (nfc_initiator_mifare_cmd (pnd, MC_WRITE, uiBlock, &mp) == false) {
        printf ("failed to write trailer block %d \n", uiBlock);
        bFailure = true;
      }
    } else {
      // The first block 0x00 is read only, skip this
      if (uiBlock == 0)
        continue;

      // Make sure a earlier write did not fail
      if (!bFailure) {
        // Try to write the data block
        memcpy (mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData, 16);
        if (!nfc_initiator_mifare_cmd (pnd, MC_WRITE, uiBlock, &mp))
          bFailure = true;
      }
    }
  }
  print_success_or_failure (bFailure, &uiWriteBlocks);
  printf ("|\n");
  printf ("Done, %d of %d blocks written.\n", uiWriteBlocks, uiBlocks + 1);
  fflush (stdout);

  return true;
}

static void
mifare_classic_extract_payload (const char *abDump, char *pbPayload)
{
  uint8_t uiSectorIndex;
  uint8_t uiBlockIndex;
  size_t  szDumpOffset;
  size_t  szPayloadIndex = 0;

  for (uiSectorIndex = 1; uiSectorIndex < 16; uiSectorIndex++) {
    for (uiBlockIndex = 0; uiBlockIndex < 3; uiBlockIndex++) {
      szDumpOffset = uiSectorIndex * 16 * 4 + uiBlockIndex * 16;
//      for(uint8_t uiByteIndex=0; uiByteIndex<16; uiByteIndex++) printf("%02x ", abDump[szPayloadIndex+uiByteIndex]);
      memcpy (pbPayload + szPayloadIndex, abDump + szDumpOffset, 16);
      szPayloadIndex += 16;
    }
  }
}

typedef enum {
  ACTION_READ,
  ACTION_WRITE,
  ACTION_EXTRACT,
  ACTION_USAGE
} action_t;

static void
print_usage (const char *pcProgramName)
{
  printf ("Usage: ");
  printf ("%s r|w a|b <dump.mfd> [<keys.mfd>]\n", pcProgramName);
  printf ("  r|w           - Perform read from (r) or write to (w) card\n");
  printf ("  a|b           - Use A or B keys for action\n");
  printf ("  <dump.mfd>    - MiFare Dump (MFD) used to write (card to MFD) or (MFD to card)\n");
  printf ("  <keys.mfd>    - MiFare Dump (MFD) that contain the keys (optional)\n");
  printf ("Or: ");
  printf ("%s x <dump.mfd> <payload.bin>\n", pcProgramName);
  printf ("  x             - Extract payload (data blocks) from MFD\n");
  printf ("  <dump.mfd>    - MiFare Dump (MFD) that contains wanted payload\n");
  printf ("  <payload.bin> - Binary file where payload will be extracted\n");
}

int
main (int argc, const char *argv[])
{
  bool    b4K;
  action_t atAction = ACTION_USAGE;
  byte_t *pbtUID;
  FILE   *pfKeys = NULL;
  FILE   *pfDump = NULL;
  const char *command = argv[1];

  if (argc < 3) {
    print_usage (argv[0]);
    exit (EXIT_FAILURE);
  }

  if (strcmp (command, "r") == 0) {
    atAction = ACTION_READ;
    bUseKeyA = tolower ((int) ((unsigned char) *(argv[2]))) == 'a';
    bUseKeyFile = (argc > 4);
  } else if (strcmp (command, "w") == 0) {
    atAction = ACTION_WRITE;
    bUseKeyA = tolower ((int) ((unsigned char) *(argv[2]))) == 'a';
    bUseKeyFile = (argc > 4);
  } else if (strcmp (command, "x") == 0) {
    atAction = ACTION_EXTRACT;
  }

  switch (atAction) {
  case ACTION_USAGE:
    print_usage (argv[0]);
    exit (EXIT_FAILURE);
    break;
  case ACTION_READ:
  case ACTION_WRITE:
    if (argc < 4) {
      print_usage (argv[0]);
      exit (EXIT_FAILURE);
    }

    if (bUseKeyFile) {
      pfKeys = fopen (argv[4], "rb");
      if (pfKeys == NULL) {
        printf ("Could not open keys file: %s\n", argv[4]);
        exit (EXIT_FAILURE);
      }
      if (fread (&mtKeys, 1, sizeof (mtKeys), pfKeys) != sizeof (mtKeys)) {
        printf ("Could not read keys file: %s\n", argv[4]);
        fclose (pfKeys);
        exit (EXIT_FAILURE);
      }
      fclose (pfKeys);
    }

    if (atAction == ACTION_READ) {
      memset (&mtDump, 0x00, sizeof (mtDump));
    } else {
      pfDump = fopen (argv[3], "rb");

      if (pfDump == NULL) {
        printf ("Could not open dump file: %s\n", argv[3]);
        exit (EXIT_FAILURE);
      }

      if (fread (&mtDump, 1, sizeof (mtDump), pfDump) != sizeof (mtDump)) {
        printf ("Could not read dump file: %s\n", argv[3]);
        fclose (pfDump);
        exit (EXIT_FAILURE);
      }
      fclose (pfDump);
    }
    // printf("Successfully opened required files\n");

    // Try to open the NFC reader
    pnd = nfc_connect (NULL);
    if (pnd == NULL) {
      printf ("Error connecting NFC reader\n");
      exit (EXIT_FAILURE);
    }

    nfc_initiator_init (pnd);

    // Drop the field for a while
    if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, false)) {
      nfc_perror (pnd, "nfc_configure");
      exit (EXIT_FAILURE);
    }
    // Let the reader only try once to find a tag
    if (!nfc_configure (pnd, NDO_INFINITE_SELECT, false)) {
      nfc_perror (pnd, "nfc_configure");
      exit (EXIT_FAILURE);
    }
    if (!nfc_configure (pnd, NDO_HANDLE_CRC, true)) {
      nfc_perror (pnd, "nfc_configure");
      exit (EXIT_FAILURE);
    }
    if (!nfc_configure (pnd, NDO_HANDLE_PARITY, true)) {
      nfc_perror (pnd, "nfc_configure");
      exit (EXIT_FAILURE);
    }
    // Enable field so more power consuming cards can power themselves up
    if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, true)) {
      nfc_perror (pnd, "nfc_configure");
      exit (EXIT_FAILURE);
    }
    // Disable ISO14443-4 switching in order to read devices that emulate Mifare Classic with ISO14443-4 compliance.
    nfc_configure (pnd, NDO_AUTO_ISO14443_4, false);

    printf ("Connected to NFC reader: %s\n", pnd->acName);

    // Try to find a MIFARE Classic tag
    if (!nfc_initiator_select_passive_target (pnd, nmMifare, NULL, 0, &nt)) {
      printf ("Error: no tag was found\n");
      nfc_disconnect (pnd);
      exit (EXIT_FAILURE);
    }
    // Test if we are dealing with a MIFARE compatible tag
    if ((nt.nti.nai.btSak & 0x08) == 0) {
      printf ("Error: tag is not a MIFARE Classic card\n");
      nfc_disconnect (pnd);
      exit (EXIT_FAILURE);
    }

    if (bUseKeyFile) {
      // Get the info from the key dump
      b4K = (mtKeys.amb[0].mbm.abtATQA[1] == 0x02);
      pbtUID = mtKeys.amb[0].mbm.abtUID;

      // Compare if key dump UID is the same as the current tag UID
      if (memcmp (nt.nti.nai.abtUid, pbtUID, 4) != 0) {
        printf ("Expected MIFARE Classic %ck card with UID: %02x%02x%02x%02x\n", b4K ? '4' : '1', pbtUID[3], pbtUID[2],
                pbtUID[1], pbtUID[0]);
      }
    }
    // Get the info from the current tag
    pbtUID = nt.nti.nai.abtUid;
    b4K = (nt.nti.nai.abtAtqa[1] == 0x02);
    printf ("Found MIFARE Classic %ck card with UID: %02x%02x%02x%02x\n", b4K ? '4' : '1', pbtUID[3], pbtUID[2],
            pbtUID[1], pbtUID[0]);

    uiBlocks = (b4K) ? 0xff : 0x3f;

    if (atAction == ACTION_READ) {
      if (read_card ()) {
        printf ("Writing data to file: %s ...", argv[3]);
        fflush (stdout);
        pfDump = fopen (argv[3], "wb");
	if (pfDump == NULL) {
	    printf ("Could not open dump file: %s\n", argv[3]);
	    exit (EXIT_FAILURE);
	}
        if (fwrite (&mtDump, 1, sizeof (mtDump), pfDump) != sizeof (mtDump)) {
          printf ("\nCould not write to file: %s\n", argv[3]);
          exit (EXIT_FAILURE);
        }
        printf ("Done.\n");
        fclose (pfDump);
      }
    } else {
      write_card ();
    }

    nfc_disconnect (pnd);
    break;

  case ACTION_EXTRACT:{
      const char *pcDump = argv[2];
      const char *pcPayload = argv[3];

      FILE   *pfDump = NULL;
      FILE   *pfPayload = NULL;

      char    abDump[4096];
      char    abPayload[4096];

      pfDump = fopen (pcDump, "rb");

      if (pfDump == NULL) {
        printf ("Could not open dump file: %s\n", pcDump);
        exit (EXIT_FAILURE);
      }

      if (fread (abDump, 1, sizeof (abDump), pfDump) != sizeof (abDump)) {
        printf ("Could not read dump file: %s\n", pcDump);
        fclose (pfDump);
        exit (EXIT_FAILURE);
      }
      fclose (pfDump);

      mifare_classic_extract_payload (abDump, abPayload);

      printf ("Writing data to file: %s\n", pcPayload);
      pfPayload = fopen (pcPayload, "wb");
      if (pfPayload == NULL) {
        printf ("Could not open file %s for writting.\n", pcPayload);
        exit (EXIT_FAILURE);
      }
      if (fwrite (abPayload, 1, sizeof (abPayload), pfPayload) != sizeof (abPayload)) {
        printf ("Could not write to file: %s\n", pcPayload);
        exit (EXIT_FAILURE);
      }
      fclose (pfPayload);
      printf ("Done, all bytes have been extracted!\n");
    }
  };

  exit (EXIT_SUCCESS);
}
