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
 * Copyright (C) 2014      Pim 't Hart
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
 * @file nfc-jewel.c
 * @brief Jewel dump/restore tool
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
#include "jewel.h"

static nfc_device *pnd;
static nfc_target nt;
static jewel_req req;
static jewel_res res;
static jewel_tag ttDump;
static uint32_t uiBlocks = 0x0E;
static uint32_t uiBytesPerBlock = 0x08;

static const nfc_modulation nmJewel = {
  .nmt = NMT_JEWEL,
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
  uint32_t   block;
  uint32_t   byte;
  bool      bFailure = false;
  uint32_t uiReadBlocks = 0;

  printf("Reading %d blocks |", uiBlocks + 1);

  for (block = 0; block <= uiBlocks; block++) {
    for (byte = 0; byte < uiBytesPerBlock; byte++) {

      // Try to read the byte
      req.read.btCmd = TC_READ;
      req.read.btAdd = (block << 3) + byte;
      if (nfc_initiator_jewel_cmd(pnd, req, &res)) {
        ttDump.ttd.abtData[(block << 3) + byte] = res.read.btDat;
      } else {
        bFailure = true;
        break;
      }
    }

    print_success_or_failure(bFailure, &uiReadBlocks);
    fflush(stdout);
  }
  printf("|\n");
  printf("Done, %d of %d blocks read.\n", uiReadBlocks, uiBlocks + 1);

  return (!bFailure);
}

static  bool
write_card(void)
{
  uint32_t block;
  uint32_t byte;
  bool     bFailure = false;
  uint32_t uiWrittenBlocks = 0;
  uint32_t uiSkippedBlocks = 0;
  uint32_t uiPartialBlocks = 0;

  char    buffer[BUFSIZ];
  bool    write_otp;
  bool    write_lock;

  printf("Write Lock bytes ? [yN] ");
  if (!fgets(buffer, BUFSIZ, stdin)) {
    ERR("Unable to read standard input.");
  }
  write_lock = ((buffer[0] == 'y') || (buffer[0] == 'Y'));

  printf("Write OTP bytes ? [yN] ");
  if (!fgets(buffer, BUFSIZ, stdin)) {
    ERR("Unable to read standard input.");
  }
  write_otp = ((buffer[0] == 'y') || (buffer[0] == 'Y'));

  printf("Writing %d pages |", uiBlocks + 1);

  // Skip block 0 - as far as I know there are no Jewel tags with block 0 writeable
  printf("s");
  uiSkippedBlocks++;

  for (block = uiSkippedBlocks; block <= uiBlocks; block++) {
    // Skip block 0x0D - it is reserved for internal use and can't be written
    if (block == 0x0D) {
      printf("s");
      uiSkippedBlocks++;
      continue;
    }
    // Skip block 0X0E if lock-bits and OTP shouldn't be written
    if ((block == 0x0E) && (!write_lock) && (!write_otp)) {
      printf("s");
      uiSkippedBlocks++;
      continue;
    }
    // Write block 0x0E partially if lock-bits or OTP shouldn't be written
    if ((block == 0x0E) && (!write_lock || !write_otp)) {
      printf("p");
      uiPartialBlocks++;
    }

    for (byte = 0; byte < uiBytesPerBlock; byte++) {
      if ((block == 0x0E) && (byte == 0 || byte == 1) && (!write_lock)) {
        continue;
      }
      if ((block == 0x0E) && (byte > 1) && (!write_otp)) {
        continue;
      }

      // Show if the readout went well
      if (bFailure) {
        // When a failure occured we need to redo the anti-collision
        if (nfc_initiator_select_passive_target(pnd, nmJewel, NULL, 0, &nt) <= 0) {
          ERR("tag was removed");
          return false;
        }
        bFailure = false;
      }

      req.writee.btCmd = TC_WRITEE;
      req.writee.btAdd = (block << 3) + byte;
      req.writee.btDat = ttDump.ttd.abtData[(block << 3) + byte];
      if (!nfc_initiator_jewel_cmd(pnd, req, &res)) {
        bFailure = true;
      }
    }
    print_success_or_failure(bFailure, &uiWrittenBlocks);
    fflush(stdout);
  }
  printf("|\n");
  printf("Done, %d of %d blocks written (%d blocks partial, %d blocks skipped).\n", uiWrittenBlocks, uiBlocks + 1, uiPartialBlocks, uiSkippedBlocks);

  return true;
}

int
main(int argc, const char *argv[])
{
  bool    bReadAction;
  FILE   *pfDump;

  if (argc < 3) {
    printf("\n");
    printf("%s r|w <dump.jwd>\n", argv[0]);
    printf("\n");
    printf("r|w         - Perform read from or write to card\n");
    printf("<dump.jwd>  - JeWel Dump (JWD) used to write (card to JWD) or (JWD to card)\n");
    printf("\n");
    exit(EXIT_FAILURE);
  }

  DBG("\nChecking arguments and settings\n");

  bReadAction = tolower((int)((unsigned char) * (argv[1])) == 'r');

  if (bReadAction) {
    memset(&ttDump, 0x00, sizeof(ttDump));
  } else {
    pfDump = fopen(argv[2], "rb");

    if (pfDump == NULL) {
      ERR("Could not open dump file: %s\n", argv[2]);
      exit(EXIT_FAILURE);
    }

    if (fread(&ttDump, 1, sizeof(ttDump), pfDump) != sizeof(ttDump)) {
      ERR("Could not read from dump file: %s\n", argv[2]);
      fclose(pfDump);
      exit(EXIT_FAILURE);
    }
    fclose(pfDump);
  }
  DBG("Successfully opened the dump file\n");

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

  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

  // Try to find a Jewel tag
  if (nfc_initiator_select_passive_target(pnd, nmJewel, NULL, 0, &nt) <= 0) {
    ERR("no tag was found\n");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  // Get the info from the current tag
  printf("Found Jewel card with UID: ");
  size_t  szPos;
  for (szPos = 0; szPos < 4; szPos++) {
    printf("%02x", nt.nti.nji.btId[szPos]);
  }
  printf("\n");

  if (bReadAction) {
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
      if (fwrite(&ttDump, 1, sizeof(ttDump), pfDump) != sizeof(ttDump)) {
        printf("Could not write to file: %s\n", argv[2]);
        fclose(pfDump);
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      fclose(pfDump);
      printf("Done.\n");
    }
  } else {
    write_card();
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
