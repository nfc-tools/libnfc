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
 * Copyright (C) 2013      Adam Laurie
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
 * @file nfc-mfultralight.c
 * @brief MIFARE Ultralight dump/restore tool
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

#include "nfc-utils.h"
#include "mifare.h"

#define MAX_TARGET_COUNT 16
#define MAX_UID_LEN 10
#define BLOCK_COUNT 0xf

static nfc_device *pnd;
static nfc_target nt;
static mifare_param mp;
static mifareul_tag mtDump;
static const uint32_t uiBlocks = BLOCK_COUNT;

// special unlock command
uint8_t  abtUnlock1[1] = { 0x40 };
uint8_t  abtUnlock2[1] = { 0x43 };

//Halt command
uint8_t  abtHalt[4] = { 0x50, 0x00, 0x00, 0x00 };

#define MAX_FRAME_LEN 264

static uint8_t abtRx[MAX_FRAME_LEN];
static int szRxBits;

static const nfc_modulation nmMifare = {
  .nmt = NMT_ISO14443A,
  .nbr = NBR_106,
};

static void
print_success_or_failure(bool bFailure, uint32_t *uiCounter)
{
  printf("%c", (bFailure) ? 'x' : '.');
  if (uiCounter)
    *uiCounter += (bFailure) ? 0 : 1;
}

static  bool
read_card(void)
{
  uint32_t page;
  bool    bFailure = false;
  uint32_t uiReadedPages = 0;

  printf("Reading %d pages |", uiBlocks + 1);

  for (page = 0; page <= uiBlocks; page += 4) {
    // Try to read out the data block
    if (nfc_initiator_mifare_cmd(pnd, MC_READ, page, &mp)) {
      memcpy(mtDump.amb[page / 4].mbd.abtData, mp.mpd.abtData, 16);
    } else {
      bFailure = true;
      break;
    }

    print_success_or_failure(bFailure, &uiReadedPages);
    print_success_or_failure(bFailure, &uiReadedPages);
    print_success_or_failure(bFailure, &uiReadedPages);
    print_success_or_failure(bFailure, &uiReadedPages);
  }
  printf("|\n");
  printf("Done, %d of %d pages read.\n", uiReadedPages, uiBlocks + 1);
  fflush(stdout);

  return (!bFailure);
}

static  bool
transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
{
  // Transmit the bit frame command, we don't use the arbitrary parity feature
  if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, NULL, abtRx, sizeof(abtRx), NULL)) < 0)
    return false;

  return true;
}


static  bool
transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
  int res;
  if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
    return false;

  return true;
}

static bool
unlock_card(void)
{
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
    return false;
  }
  if (!transmit_bytes(abtUnlock2, 1)) {
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

static bool check_magic() {
    bool     bFailure = false;
    int      uid_data;

    for (uint32_t page = 0; page <= 1; page++) {
    // Show if the readout went well
    if (bFailure) {
      // When a failure occured we need to redo the anti-collision
      if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
        ERR("tag was removed");
        return false;
      }
      bFailure = false;
    }

    uid_data = 0x00000000;

    memcpy(mp.mpd.abtData, &uid_data, sizeof uid_data);
    memset(mp.mpd.abtData + 4, 0, 12);

    //Force the write without checking for errors - otherwise the writes to the sector 0 seem to complain
    nfc_initiator_mifare_cmd(pnd, MC_WRITE, page, &mp);
  }

    //Check that the ID is now set to 0x000000000000
    if (nfc_initiator_mifare_cmd(pnd, MC_READ, 0, &mp)) {
        //printf("%u", mp.mpd.abtData);
          bool result = true;
          for(int i = 0; i <= 7; i++) {
           if (mp.mpd.abtData[i] != 0x00) result = false;
          }

      if (result) {
        return true;
      }

    }

    //Initially check if we can unlock via the MF method
    if (unlock_card()) {
      return true;
    } else {
      return false;
    }

}

static  bool
write_card(bool write_otp, bool write_lock, bool write_uid)
{
  uint32_t uiBlock = 0;
  bool    bFailure = false;
  uint32_t uiWritenPages = 0;
  uint32_t uiSkippedPages = 0;

  char    buffer[BUFSIZ];

  if (!write_otp) {
    printf("Write OTP bytes ? [yN] ");
    if (!fgets(buffer, BUFSIZ, stdin)) {
      ERR("Unable to read standard input.");
    }
    write_otp = ((buffer[0] == 'y') || (buffer[0] == 'Y'));
  }

  if (!write_lock) {
    printf("Write Lock bytes ? [yN] ");
    if (!fgets(buffer, BUFSIZ, stdin)) {
      ERR("Unable to read standard input.");
    }
    write_lock = ((buffer[0] == 'y') || (buffer[0] == 'Y'));
  }

  if (!write_uid) {
    printf("Write UID bytes (only for special writeable UID cards) ? [yN] ");
    if (!fgets(buffer, BUFSIZ, stdin)) {
      ERR("Unable to read standard input.");
    }
    write_uid = ((buffer[0] == 'y') || (buffer[0] == 'Y'));
  }

  printf("Writing %d pages |", uiBlocks + 1);
  /* We may need to skip 2 first pages. */
  if (!write_uid) {
    printf("ss");
    uiSkippedPages = 2;
  } else {
    if (!check_magic()) {
      printf("\nUnable to unlock card - are you sure the card is magic?\n");
      bFailure = false;
      return false;
    }
  }

  for (uint32_t page = uiSkippedPages; page <= ((uiBlocks / 4) * 4); page++) {
    if ((page == 0x2) && (!write_lock)) {
      printf("s");
      uiSkippedPages++;
      continue;
    }
    if ((page == 0x3) && (!write_otp)) {
      printf("s");
      uiSkippedPages++;
      continue;
    }
    // Show if the readout went well
    if (bFailure) {
      // When a failure occured we need to redo the anti-collision
      if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0) {
        ERR("tag was removed");
        return false;
      }
      bFailure = false;
    }
    // For the Mifare Ultralight, this write command can be used
    // in compatibility mode, which only actually writes the first
    // page (4 bytes). The Ultralight-specific Write command only
    // writes one page at a time.
    uiBlock = page / 4;
    memcpy(mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData + ((page % 4) * 4), 4);
    memset(mp.mpd.abtData + 4, 0, 12);
    if (!nfc_initiator_mifare_cmd(pnd, MC_WRITE, page, &mp))
      bFailure = true;

    print_success_or_failure(bFailure, &uiWritenPages);
  }
  printf("|\n");
  printf("Done, %d of %d pages written (%d pages skipped).\n", uiWritenPages, uiBlocks + 1, uiSkippedPages);

  return true;
}

static int list_passive_targets(nfc_device *_pnd)
{
  int res = 0;

  nfc_target ant[MAX_TARGET_COUNT];

  if (nfc_initiator_init(_pnd) < 0) {
    return -EXIT_FAILURE;
  }

  if ((res = nfc_initiator_list_passive_targets(_pnd, nmMifare, ant, MAX_TARGET_COUNT)) >= 0) {
    int i;

    if (res > 0)
      printf("%d ISO14443A passive target(s) found:\n", res);

    for (i = 0; i < res; i++) {
      size_t  szPos;

      printf("\t");
      for (szPos = 0; szPos < ant[i].nti.nai.szUidLen; szPos++) {
        printf("%02x", ant[i].nti.nai.abtUid[szPos]);
      }
      printf("\n");
    }

  }

  return 0;
}

static size_t str_to_uid(const char *str, uint8_t *uid)
{
  uint8_t i;

  memset(uid, 0x0, MAX_UID_LEN);
  i = 0;
  while ((*str != '\0') && ((i >> 1) < MAX_UID_LEN) ) {
    char nibble[2] = { 0x00, '\n' }; /* for strtol */

    nibble[0] = *str++;
    if (isxdigit(nibble[0])) {
      if (isupper(nibble[0]))
        nibble[0] = tolower(nibble[0]);
      uid[i >> 1] |= strtol(nibble, NULL, 16) << ((i % 2) ? 0 : 4) & ((i % 2) ? 0x0f : 0xf0);
      i++;
    }
  }
  return i >> 1;
}

static void
print_usage(const char *argv[])
{
  printf("Usage: %s r|w <dump.mfd> [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\tr|w\t\t - Perform read or write\n");
  printf("\t<dump.mfd>\t - MiFare Dump (MFD) used to write (card to MFD) or (MFD to card)\n");
  printf("\t--otp\t\t - Don't prompt for OTP writing (Assume yes)\n");
  printf("\t--lock\t\t - Don't prompt for Lockbit writing (Assume yes)\n");
  printf("\t--uid\t\t - Don't prompt for UID writing (Assume yes)\n");
  printf("\t--full\t\t - Assume full card write (UID + OTP + Lockbit)\n");
  printf("\t--with-uid <UID>\t\t - Specify UID to read/write from\n");
}

int
main(int argc, const char *argv[])
{
  int     iAction = 0;
  uint8_t iUID[MAX_UID_LEN] = { 0x0 };
  size_t  szUID = 0;
  bool    bOTP = false;
  bool    bLock = false;
  bool    bUID = false;
  FILE   *pfDump;

  if (argc < 2) {
      print_usage(argv);
      exit(EXIT_FAILURE);
  }

  DBG("\nChecking arguments and settings\n");

  // Get commandline options
  for (int arg = 1; arg < argc; arg++) {
    if (0 == strcmp(argv[arg], "r")) {
      iAction = 1;
    } else if (0 == strcmp(argv[arg], "w")) {
      iAction = 2;
    } else if (0 == strcmp(argv[arg], "--with-uid")) {
      if (argc < 5) {
        ERR("Please supply a UID of 4, 7 or 10 bytes long. Ex: a1:b2:c3:d4");
        exit(EXIT_FAILURE);
      }
      szUID = str_to_uid(argv[4], iUID);
    } else if (0 == strcmp(argv[arg], "--full")) {
      bOTP = true;
      bLock = true;
      bUID = true;
    } else if (0 == strcmp(argv[arg], "--otp")) {
      bOTP = true;
    } else if (0 == strcmp(argv[arg], "--lock")) {
      bLock = true;
    } else if (0 == strcmp(argv[arg], "--uid")) {
      bUID = true;
    } else if (0 == strcmp(argv[arg], "--check-magic")) {
      iAction = 3;
    } else {
      //Skip validation of the filename
      if ((arg != 2) && (arg != 4)) {
        ERR("%s is not supported option.", argv[arg]);
        print_usage(argv);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (iAction == 1) {
    memset(&mtDump, 0x00, sizeof(mtDump));
  } else if (iAction == 2) {
    pfDump = fopen(argv[2], "rb");

    if (pfDump == NULL) {
      ERR("Could not open dump file: %s\n", argv[2]);
      exit(EXIT_FAILURE);
    }

    if (fread(&mtDump, 1, sizeof(mtDump), pfDump) != sizeof(mtDump)) {
      ERR("Could not read from dump file: %s\n", argv[2]);
      fclose(pfDump);
      exit(EXIT_FAILURE);
    }
    fclose(pfDump);
    DBG("Successfully opened the dump file\n");
  } else if (iAction == 3) {
    DBG("Switching to Check Magic Mode\n");
  } else {
    ERR("Unable to determine operating mode");
    exit(EXIT_FAILURE);
  }

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  // Try to open the NFC device
  pnd = nfc_open(context, NULL);
  if (pnd == NULL) {
    ERR("Error opening NFC device");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

  if (list_passive_targets(pnd)) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  // Let the device only try once to find a tag
  if (nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  // Try to find a MIFARE Ultralight tag
  if (nfc_initiator_select_passive_target(pnd, nmMifare, (szUID) ? iUID : NULL, szUID, &nt) <= 0) {
    ERR("no tag was found\n");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  // Test if we are dealing with a MIFARE compatible tag

  if (nt.nti.nai.abtAtqa[1] != 0x44) {
    ERR("tag is not a MIFARE Ultralight card\n");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
  // Get the info from the current tag
  printf("Using MIFARE Ultralight card with UID: ");
  size_t  szPos;
  for (szPos = 0; szPos < nt.nti.nai.szUidLen; szPos++) {
    printf("%02x", nt.nti.nai.abtUid[szPos]);
  }
  printf("\n");

  if (iAction == 1) {
    if (read_card()) {
      printf("Writing data to file: %s ... ", argv[2]);
      fflush(stdout);
      pfDump = fopen(argv[2], "wb");
      if (pfDump == NULL) {
        printf("Could not open file: %s\n", argv[2]);
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (fwrite(&mtDump, 1, sizeof(mtDump), pfDump) != sizeof(mtDump)) {
        printf("Could not write to file: %s\n", argv[2]);
        fclose(pfDump);
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      fclose(pfDump);
      printf("Done.\n");
    }
  } else if (iAction == 2) {
    write_card(bOTP, bLock, bUID);
  } else if (iAction == 3) {
    if (!check_magic()) {
        printf("Card is not magic\n");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
    } else {
        printf("Card is magic\n");
    }
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
