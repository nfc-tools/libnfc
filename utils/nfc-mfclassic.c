/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2011-2013 Adam Laurie
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

static nfc_context *context;
static nfc_device *pnd;
static nfc_target nt;
static mifare_param mp;
static mifare_classic_tag mtKeys;
static mifare_classic_tag mtDump;
static bool bUseKeyA;
static bool bUseKeyFile;
static bool bForceKeyFile;
static bool bTolerateFailures;
static bool bFormatCard;
static bool magic2 = false;
static uint8_t uiBlocks;
static uint8_t keys[] = {
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
static uint8_t default_key[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t default_acl[] = {0xff, 0x07, 0x80, 0x69};

static const nfc_modulation nmMifare = {
  .nmt = NMT_ISO14443A,
  .nbr = NBR_106,
};

static size_t num_keys = sizeof(keys) / 6;

#define MAX_FRAME_LEN 264

static uint8_t abtRx[MAX_FRAME_LEN];
static int szRxBits;

uint8_t  abtHalt[4] = { 0x50, 0x00, 0x00, 0x00 };

// special unlock command
uint8_t  abtUnlock1[1] = { 0x40 };
uint8_t  abtUnlock2[1] = { 0x43 };

static  bool
transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
{
  // Show transmitted command
  printf("Sent bits:     ");
  print_hex_bits(pbtTx, szTxBits);
  // Transmit the bit frame command, we don't use the arbitrary parity feature
  if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, NULL, abtRx, sizeof(abtRx), NULL)) < 0)
    return false;

  // Show received answer
  printf("Received bits: ");
  print_hex_bits(abtRx, szRxBits);
  // Succesful transfer
  return true;
}


static  bool
transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
  // Show transmitted command
  printf("Sent bits:     ");
  print_hex(pbtTx, szTx);
  // Transmit the command bytes
  int res;
  if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
    return false;

  // Show received answer
  printf("Received bits: ");
  print_hex(abtRx, res);
  // Succesful transfer
  return true;
}

static void
print_success_or_failure(bool bFailure, uint32_t *uiBlockCounter)
{
  printf("%c", (bFailure) ? 'x' : '.');
  if (uiBlockCounter && !bFailure)
    *uiBlockCounter += 1;
}

static  bool
is_first_block(uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock) % 4 == 0);
  else
    return ((uiBlock) % 16 == 0);
}

static  bool
is_trailer_block(uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

static  uint32_t
get_trailer_block(uint32_t uiFirstBlock)
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
authenticate(uint32_t uiBlock)
{
  mifare_cmd mc;
  uint32_t uiTrailerBlock;

  // Set the authentication information (uid)
  memcpy(mp.mpa.abtAuthUid, nt.nti.nai.abtUid + nt.nti.nai.szUidLen - 4, 4);

  // Should we use key A or B?
  mc = (bUseKeyA) ? MC_AUTH_A : MC_AUTH_B;

  // Key file authentication.
  if (bUseKeyFile) {

    // Locate the trailer (with the keys) used for this sector
    uiTrailerBlock = get_trailer_block(uiBlock);

    // Extract the right key from dump file
    if (bUseKeyA)
      memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyA, 6);
    else
      memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyB, 6);

    // Try to authenticate for the current sector
    if (nfc_initiator_mifare_cmd(pnd, mc, uiBlock, &mp))
      return true;
  }

  // If formatting or not using key file, try to guess the right key
  if (bFormatCard || !bUseKeyFile) {
    for (size_t key_index = 0; key_index < num_keys; key_index++) {
      memcpy(mp.mpa.abtKey, keys + (key_index * 6), 6);
      if (nfc_initiator_mifare_cmd(pnd, mc, uiBlock, &mp)) {
        if (bUseKeyA)
          memcpy(mtKeys.amb[uiBlock].mbt.abtKeyA, &mp.mpa.abtKey, 6);
        else
          memcpy(mtKeys.amb[uiBlock].mbt.abtKeyB, &mp.mpa.abtKey, 6);
        return true;
      }
      if (nfc_initiator_select_passive_target(pnd, nmMifare, nt.nti.nai.abtUid, nt.nti.nai.szUidLen, NULL) <= 0) {
        ERR("tag was removed");
        return false;
      }
    }
  }

  return false;
}

static bool
unlock_card(void)
{
  if (magic2) {
    printf("Don't use R/W with this card, this is not required!\n");
    return false;
  }

  // Configure the CRC
  if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0) {
    nfc_perror(pnd, "nfc_configure");
    return false;
  }
  // Use raw send/receive methods
  if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) {
    nfc_perror(pnd, "nfc_configure");
    return false;
  }

  iso14443a_crc_append(abtHalt, 2);
  transmit_bytes(abtHalt, 4);
  // now send unlock
  if (!transmit_bits(abtUnlock1, 7)) {
    printf("unlock failure!\n");
    return false;
  }
  if (!transmit_bytes(abtUnlock2, 1)) {
    printf("unlock failure!\n");
    return false;
  }

  // reset reader
  // Configure the CRC
  if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    return false;
  }
  // Switch off raw send/receive methods
  if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    return false;
  }
  return true;
}

static int
get_rats(void)
{
  int res;
  uint8_t  abtRats[2] = { 0xe0, 0x50};
  // Use raw send/receive methods
  if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) {
    nfc_perror(pnd, "nfc_configure");
    return -1;
  }
  res = nfc_initiator_transceive_bytes(pnd, abtRats, sizeof(abtRats), abtRx, sizeof(abtRx), 0);
  if (res > 0) {
    // ISO14443-4 card, turn RF field off/on to access ISO14443-3 again
    if (nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false) < 0) {
      nfc_perror(pnd, "nfc_configure");
      return -1;
    }
    if (nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true) < 0) {
      nfc_perror(pnd, "nfc_configure");
      return -1;
    }
  }
  // Reselect tag
  if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
    printf("Error: tag disappeared\n");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  return res;
}

static  bool
read_card(int read_unlocked)
{
  int32_t iBlock;
  bool    bFailure = false;
  uint32_t uiReadBlocks = 0;

  if (read_unlocked)
    if (!unlock_card())
      return false;

  printf("Reading out %d blocks |", uiBlocks + 1);
  // Read the card from end to begin
  for (iBlock = uiBlocks; iBlock >= 0; iBlock--) {
    // Authenticate everytime we reach a trailer block
    if (is_trailer_block(iBlock)) {
      if (bFailure) {
        // When a failure occured we need to redo the anti-collision
        if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
          printf("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      }

      fflush(stdout);

      // Try to authenticate for the current sector
      if (!read_unlocked && !authenticate(iBlock)) {
        printf("!\nError: authentication failed for block 0x%02x\n", iBlock);
        return false;
      }
      // Try to read out the trailer
      if (nfc_initiator_mifare_cmd(pnd, MC_READ, iBlock, &mp)) {
        if (read_unlocked) {
          memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, 16);
        } else {
          // Copy the keys over from our key dump and store the retrieved access bits
          memcpy(mtDump.amb[iBlock].mbt.abtKeyA, mtKeys.amb[iBlock].mbt.abtKeyA, 6);
          memcpy(mtDump.amb[iBlock].mbt.abtAccessBits, mp.mpd.abtData + 6, 4);
          memcpy(mtDump.amb[iBlock].mbt.abtKeyB, mtKeys.amb[iBlock].mbt.abtKeyB, 6);
        }
      } else {
        printf("!\nfailed to read trailer block 0x%02x\n", iBlock);
        bFailure = true;
      }
    } else {
      // Make sure a earlier readout did not fail
      if (!bFailure) {
        // Try to read out the data block
        if (nfc_initiator_mifare_cmd(pnd, MC_READ, iBlock, &mp)) {
          memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, 16);
        } else {
          printf("!\nError: unable to read block 0x%02x\n", iBlock);
          bFailure = true;
        }
      }
    }
    // Show if the readout went well for each block
    print_success_or_failure(bFailure, &uiReadBlocks);
    if ((! bTolerateFailures) && bFailure)
      return false;
  }
  printf("|\n");
  printf("Done, %d of %d blocks read.\n", uiReadBlocks, uiBlocks + 1);
  fflush(stdout);

  return true;
}

static  bool
write_card(int write_block_zero)
{
  uint32_t uiBlock;
  bool    bFailure = false;
  uint32_t uiWriteBlocks = 0;

  if (write_block_zero)
    if (!unlock_card())
      return false;

  printf("Writing %d blocks |", uiBlocks + 1);
  // Write the card from begin to end;
  for (uiBlock = 0; uiBlock <= uiBlocks; uiBlock++) {
    // Authenticate everytime we reach the first sector of a new block
    if (is_first_block(uiBlock)) {
      if (bFailure) {
        // When a failure occured we need to redo the anti-collision
        if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
          printf("!\nError: tag was removed\n");
          return false;
        }
        bFailure = false;
      }

      fflush(stdout);

      // Try to authenticate for the current sector
      if (!write_block_zero && !authenticate(uiBlock)) {
        printf("!\nError: authentication failed for block %02x\n", uiBlock);
        return false;
      }
    }

    if (is_trailer_block(uiBlock)) {
      if (bFormatCard) {
        // Copy the default key and reset the access bits
        memcpy(mp.mpd.abtData, default_key, 6);
        memcpy(mp.mpd.abtData + 6, default_acl, 4);
        memcpy(mp.mpd.abtData + 10, default_key, 6);
      } else {
        // Copy the keys over from our key dump and store the retrieved access bits
        memcpy(mp.mpd.abtData, mtDump.amb[uiBlock].mbt.abtKeyA, 6);
        memcpy(mp.mpd.abtData + 6, mtDump.amb[uiBlock].mbt.abtAccessBits, 4);
        memcpy(mp.mpd.abtData + 10, mtDump.amb[uiBlock].mbt.abtKeyB, 6);
      }

      // Try to write the trailer
      if (nfc_initiator_mifare_cmd(pnd, MC_WRITE, uiBlock, &mp) == false) {
        printf("failed to write trailer block %d \n", uiBlock);
        bFailure = true;
      }
    } else {
      // The first block 0x00 is read only, skip this
      if (uiBlock == 0 && ! write_block_zero && ! magic2)
        continue;


      // Make sure a earlier write did not fail
      if (!bFailure) {
        // Try to write the data block
        if (bFormatCard && uiBlock)
          memset(mp.mpd.abtData, 0x00, 16);
        else
          memcpy(mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData, 16);
        // do not write a block 0 with incorrect BCC - card will be made invalid!
        if (uiBlock == 0) {
          if ((mp.mpd.abtData[0] ^ mp.mpd.abtData[1] ^ mp.mpd.abtData[2] ^ mp.mpd.abtData[3] ^ mp.mpd.abtData[4]) != 0x00 && !magic2) {
            printf("!\nError: incorrect BCC in MFD file!\n");
            printf("Expecting BCC=%02X\n", mp.mpd.abtData[0] ^ mp.mpd.abtData[1] ^ mp.mpd.abtData[2] ^ mp.mpd.abtData[3]);
            return false;
          }
        }
        if (!nfc_initiator_mifare_cmd(pnd, MC_WRITE, uiBlock, &mp))
          bFailure = true;
      }
    }
    // Show if the write went well for each block
    print_success_or_failure(bFailure, &uiWriteBlocks);
    if ((! bTolerateFailures) && bFailure)
      return false;
  }
  printf("|\n");
  printf("Done, %d of %d blocks written.\n", uiWriteBlocks, uiBlocks + 1);
  fflush(stdout);

  return true;
}

typedef enum {
  ACTION_READ,
  ACTION_WRITE,
  ACTION_USAGE
} action_t;

static void
print_usage(const char *pcProgramName)
{
  printf("Usage: ");
  printf("%s f|r|R|w|W a|b <dump.mfd> [<keys.mfd> [f]]\n", pcProgramName);
  printf("  f|r|R|w|W     - Perform format (f) or read from (r) or unlocked read from (R) or write to (w) or unlocked write to (W) card\n");
  printf("                  *** format will reset all keys to FFFFFFFFFFFF and all data to 00 and all ACLs to default\n");
  printf("                  *** unlocked read does not require authentication and will reveal A and B keys\n");
  printf("                  *** note that unlocked write will attempt to overwrite block 0 including UID\n");
  printf("                  *** unlocking only works with special Mifare 1K cards (Chinese clones)\n");
  printf("  a|A|b|B       - Use A or B keys for action; Halt on errors (a|b) or tolerate errors (A|B)\n");
  printf("  <dump.mfd>    - MiFare Dump (MFD) used to write (card to MFD) or (MFD to card)\n");
  printf("  <keys.mfd>    - MiFare Dump (MFD) that contain the keys (optional)\n");
  printf("  f             - Force using the keyfile even if UID does not match (optional)\n");
  printf("Examples: \n\n");
  printf("  Read card to file, using key A:\n\n");
  printf("    %s r a mycard.mfd\n\n", pcProgramName);
  printf("  Write file to blank card, using key A:\n\n");
  printf("    %s w a mycard.mfd\n\n", pcProgramName);
  printf("  Write new data and/or keys to previously written card, using key A:\n\n");
  printf("    %s w a newdata.mfd mycard.mfd\n\n", pcProgramName);
  printf("  Format/wipe card (note two passes required to ensure writes for all ACL cases):\n\n");
  printf("    %s f A dummy.mfd keyfile.mfd f\n", pcProgramName);
  printf("    %s f B dummy.mfd keyfile.mfd f\n\n", pcProgramName);
}

int
main(int argc, const char *argv[])
{
  action_t atAction = ACTION_USAGE;
  uint8_t *pbtUID;
  int    unlock = 0;

  if (argc < 2) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  const char *command = argv[1];

  if (strcmp(command, "r") == 0 || strcmp(command, "R") == 0) {
    if (argc < 4) {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    atAction = ACTION_READ;
    if (strcmp(command, "R") == 0)
      unlock = 1;
    bUseKeyA = tolower((int)((unsigned char) * (argv[2]))) == 'a';
    bTolerateFailures = tolower((int)((unsigned char) * (argv[2]))) != (int)((unsigned char) * (argv[2]));
    bUseKeyFile = (argc > 4);
    bForceKeyFile = ((argc > 5) && (strcmp((char *)argv[5], "f") == 0));
  } else if (strcmp(command, "w") == 0 || strcmp(command, "W") == 0 || strcmp(command, "f") == 0) {
    if (argc < 4) {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    atAction = ACTION_WRITE;
    if (strcmp(command, "W") == 0)
      unlock = 1;
    bFormatCard = (strcmp(command, "f") == 0);
    bUseKeyA = tolower((int)((unsigned char) * (argv[2]))) == 'a';
    bTolerateFailures = tolower((int)((unsigned char) * (argv[2]))) != (int)((unsigned char) * (argv[2]));
    bUseKeyFile = (argc > 4);
    bForceKeyFile = ((argc > 5) && (strcmp((char *)argv[5], "f") == 0));
  }

  if (atAction == ACTION_USAGE) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  // We don't know yet the card size so let's read only the UID from the keyfile for the moment
  if (bUseKeyFile) {
    FILE *pfKeys = fopen(argv[4], "rb");
    if (pfKeys == NULL) {
      printf("Could not open keys file: %s\n", argv[4]);
      exit(EXIT_FAILURE);
    }
    if (fread(&mtKeys, 1, 4, pfKeys) != 4) {
      printf("Could not read UID from key file: %s\n", argv[4]);
      fclose(pfKeys);
      exit(EXIT_FAILURE);
    }
    fclose(pfKeys);
  }
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

// Try to open the NFC reader
  pnd = nfc_open(context, NULL);
  if (pnd == NULL) {
    ERR("Error opening NFC reader");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  };

// Let the reader only try once to find a tag
  if (nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
// Disable ISO14443-4 switching in order to read devices that emulate Mifare Classic with ISO14443-4 compliance.
  if (nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, false) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

// Try to find a MIFARE Classic tag
  if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
    printf("Error: no tag was found\n");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
// Test if we are dealing with a MIFARE compatible tag
  if ((nt.nti.nai.btSak & 0x08) == 0) {
    printf("Warning: tag is probably not a MFC!\n");
  }

// Get the info from the current tag
  pbtUID = nt.nti.nai.abtUid;

  if (bUseKeyFile) {
    uint8_t fileUid[4];
    memcpy(fileUid, mtKeys.amb[0].mbm.abtUID, 4);
// Compare if key dump UID is the same as the current tag UID, at least for the first 4 bytes
    if (memcmp(pbtUID, fileUid, 4) != 0) {
      printf("Expected MIFARE Classic card with UID starting as: %02x%02x%02x%02x\n",
             fileUid[0], fileUid[1], fileUid[2], fileUid[3]);
      printf("Got card with UID starting as:                     %02x%02x%02x%02x\n",
             pbtUID[0], pbtUID[1], pbtUID[2], pbtUID[3]);
      if (! bForceKeyFile) {
        printf("Aborting!\n");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
    }
  }
  printf("Found MIFARE Classic card:\n");
  print_nfc_target(&nt, false);

// Guessing size
  if ((nt.nti.nai.abtAtqa[1] & 0x02) == 0x02)
// 4K
    uiBlocks = 0xff;
  else if ((nt.nti.nai.btSak & 0x01) == 0x01)
// 320b
    uiBlocks = 0x13;
  else
// 1K/2K, checked through RATS
    uiBlocks = 0x3f;
// Testing RATS
  int res;
  if ((res = get_rats()) > 0) {
    if ((res >= 10) && (abtRx[5] == 0xc1) && (abtRx[6] == 0x05)
        && (abtRx[7] == 0x2f) && (abtRx[8] == 0x2f)
        && ((nt.nti.nai.abtAtqa[1] & 0x02) == 0x00)) {
      // MIFARE Plus 2K
      uiBlocks = 0x7f;
    }
    // Chinese magic emulation card, ATS=0978009102:dabc1910
    if ((res == 9)  && (abtRx[5] == 0xda) && (abtRx[6] == 0xbc)
        && (abtRx[7] == 0x19) && (abtRx[8] == 0x10)) {
      magic2 = true;
    }
  }
  printf("Guessing size: seems to be a %i-byte card\n", (uiBlocks + 1) * 16);

  if (bUseKeyFile) {
    FILE *pfKeys = fopen(argv[4], "rb");
    if (pfKeys == NULL) {
      printf("Could not open keys file: %s\n", argv[4]);
      exit(EXIT_FAILURE);
    }
    if (fread(&mtKeys, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfKeys) != (uiBlocks + 1) * sizeof(mifare_classic_block)) {
      printf("Could not read keys file: %s\n", argv[4]);
      fclose(pfKeys);
      exit(EXIT_FAILURE);
    }
    fclose(pfKeys);
  }

  if (atAction == ACTION_READ) {
    memset(&mtDump, 0x00, sizeof(mtDump));
  } else {
    FILE *pfDump = fopen(argv[3], "rb");

    if (pfDump == NULL) {
      printf("Could not open dump file: %s\n", argv[3]);
      exit(EXIT_FAILURE);

    }

    if (fread(&mtDump, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfDump) != (uiBlocks + 1) * sizeof(mifare_classic_block)) {
      printf("Could not read dump file: %s\n", argv[3]);
      fclose(pfDump);
      exit(EXIT_FAILURE);
    }
    fclose(pfDump);
  }
// printf("Successfully opened required files\n");

  if (atAction == ACTION_READ) {
    if (read_card(unlock)) {
      printf("Writing data to file: %s ...", argv[3]);
      fflush(stdout);
      FILE *pfDump = fopen(argv[3], "wb");
      if (pfDump == NULL) {
        printf("Could not open dump file: %s\n", argv[3]);
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (fwrite(&mtDump, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfDump) != ((uiBlocks + 1) * sizeof(mifare_classic_block))) {
        printf("\nCould not write to file: %s\n", argv[3]);
        fclose(pfDump);
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      printf("Done.\n");
      fclose(pfDump);
    }
  } else if (atAction == ACTION_WRITE) {
    write_card(unlock);
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
