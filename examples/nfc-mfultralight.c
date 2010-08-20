/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult, 2010, Romuald Conty
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
 * @file nfc-mfultool.c
 * @brief MIFARE Ultralight dump tool
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
#include <nfc/nfc-messages.h>

#include "mifare.h"
#include "nfc-utils.h"

static nfc_device_t *pnd;
static nfc_target_info_t nti;
static mifare_param mp;
static mifareul_tag mtDump;
static uint32_t uiBlocks = 0xF;

static void
print_success_or_failure (bool bFailure, uint32_t * uiCounter)
{
  printf ("%c", (bFailure) ? 'x' : '.');
  if (uiCounter)
    *uiCounter += (bFailure) ? 0 : 1;
}

static bool
read_card (void)
{
  uint32_t page;
  bool bFailure = false;
  uint32_t uiReadedPages = 0;

  printf ("Reading %d pages |", uiBlocks + 1);

  for (page = 0; page <= uiBlocks; page += 4) {
    // Try to read out the data block
    if (nfc_initiator_mifare_cmd (pnd, MC_READ, page, &mp)) {
      memcpy (mtDump.amb[page / 4].mbd.abtData, mp.mpd.abtData, 16);
    } else {
      bFailure = true;
      break;
    }

    print_success_or_failure (bFailure, &uiReadedPages);
    print_success_or_failure (bFailure, &uiReadedPages);
    print_success_or_failure (bFailure, &uiReadedPages);
    print_success_or_failure (bFailure, &uiReadedPages);
  }
  printf ("|\n");
  printf ("Done, %d of %d pages readed.\n", uiReadedPages, uiBlocks + 1);
  fflush (stdout);

  return (!bFailure);
}

static bool
write_card (void)
{
  uint32_t uiBlock = 0;
  int page = 0x4;
  bool bFailure = false;
  uint32_t uiWritenPages = 0;

  char buffer[BUFSIZ];
  bool write_otp;

  printf ("Write OTP bytes ? [yN] ");
  fgets (buffer, BUFSIZ, stdin);
  write_otp = ((buffer[0] == 'y') || (buffer[0] == 'Y'));

  /* We need to skip 3 first pages. */
  printf ("Writing %d pages |", uiBlocks + 1);
  printf ("sss");

  if(write_otp) {
    page = 0x3;
  } else {
    /* If user don't want to write OTP, we skip 1 page more. */
    printf("s");
    page = 0x4;
  }

  for (; page <= 0xF; page++) {
    // Show if the readout went well
    if (bFailure) {
      // When a failure occured we need to redo the anti-collision
      if (!nfc_initiator_select_passive_target (pnd, NM_ISO14443A_106, NULL, 0, &nti)) {
        ERR ("tag was removed");
        return false;
      }
      bFailure = false;
    }

    // For the Mifare Ultralight, this write command can be used
    // in compatibility mode, which only actually writes the first 
    // page (4 bytes). The Ultralight-specific Write command only
    // writes one page at a time.
    uiBlock = page / 4;
    memcpy (mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData + ((page % 4) * 4), 16);
    if (!nfc_initiator_mifare_cmd (pnd, MC_WRITE, page, &mp))
        bFailure = true;

    print_success_or_failure (bFailure, &uiWritenPages);
  }
  printf ("|\n");
  printf ("Done, %d of %d pages written (%d first pages are skipped).\n", uiWritenPages, uiBlocks + 1, write_otp?3:4);

  return true;
}

int
main (int argc, const char *argv[])
{
  bool bReadAction;
  byte_t *pbtUID;
  FILE *pfDump;

  if (argc < 3) {
    printf ("\n");
    printf ("%s r|w <dump.mfd>\n", argv[0]);
    printf ("\n");
    printf ("r|w         - Perform read from or write to card\n");
    printf ("<dump.mfd>  - MiFare Dump (MFD) used to write (card to MFD) or (MFD to card)\n");
    printf ("\n");
    return 1;
  }

  DBG ("\nChecking arguments and settings\n");

  bReadAction = tolower ((int) ((unsigned char) *(argv[1])) == 'r');

  if (bReadAction) {
    memset (&mtDump, 0x00, sizeof (mtDump));
  } else {
    pfDump = fopen (argv[2], "rb");

    if (pfDump == NULL) {
      ERR ("Could not open dump file: %s\n", argv[2]);
      return 1;
    }

    if (fread (&mtDump, 1, sizeof (mtDump), pfDump) != sizeof (mtDump)) {
      ERR ("Could not read from dump file: %s\n", argv[2]);
      fclose (pfDump);
      return 1;
    }
    fclose (pfDump);
  }
  DBG ("Successfully opened the dump file\n");

  // Try to open the NFC reader
  pnd = nfc_connect (NULL);
  if (pnd == NULL) {
    ERR ("Error connecting NFC reader\n");
    return 1;
  }

  nfc_initiator_init (pnd);

  // Drop the field for a while
  if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, false)) {
    nfc_perror(pnd, "nfc_configure");
    exit(EXIT_FAILURE);
  }

  // Let the reader only try once to find a tag
  if (!nfc_configure (pnd, NDO_INFINITE_SELECT, false)) {
    nfc_perror(pnd, "nfc_configure");
    exit(EXIT_FAILURE);
  }
  if (!nfc_configure (pnd, NDO_HANDLE_CRC, true)) {
    nfc_perror(pnd, "nfc_configure");
    exit(EXIT_FAILURE);
  }
  if (!nfc_configure (pnd, NDO_HANDLE_PARITY, true)) {
    nfc_perror(pnd, "nfc_configure");
    exit(EXIT_FAILURE);
  }

  // Enable field so more power consuming cards can power themselves up
  if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, true)) {
    nfc_perror(pnd, "nfc_configure");
    exit(EXIT_FAILURE);
  }

  printf ("Connected to NFC reader: %s\n", pnd->acName);

  // Try to find a MIFARE Ultralight tag
  if (!nfc_initiator_select_passive_target (pnd, NM_ISO14443A_106, NULL, 0, &nti)) {
    ERR ("no tag was found\n");
    nfc_disconnect (pnd);
    return 1;
  }
  // Test if we are dealing with a MIFARE compatible tag

  if (nti.nai.abtAtqa[1] != 0x44) {
    ERR ("tag is not a MIFARE Ultralight card\n");
    nfc_disconnect (pnd);
    return EXIT_FAILURE;
  }

  // Get the info from the current tag (UID is stored little-endian)
  pbtUID = nti.nai.abtUid;
  printf ("Found MIFARE Ultralight card with UID: %02x%02x%02x%02x\n", pbtUID[3], pbtUID[2], pbtUID[1], pbtUID[0]);

  if (bReadAction) {
    if (read_card ()) {
      printf ("Writing data to file: %s ... ", argv[2]);
      fflush (stdout);
      pfDump = fopen (argv[2], "wb");
      if (pfDump == NULL) {
        printf ("Could not open file: %s\n", argv[2]);
        return EXIT_FAILURE;
      }
      if (fwrite (&mtDump, 1, sizeof (mtDump), pfDump) != sizeof (mtDump)) {
        printf ("Could not write to file: %s\n", argv[2]);
        return EXIT_FAILURE;
      }
      fclose (pfDump);
      printf ("Done.\n");
    }
  } else {
    write_card ();
  }

  nfc_disconnect (pnd);

  return EXIT_SUCCESS;
}
