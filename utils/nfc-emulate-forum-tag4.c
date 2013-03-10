/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Additional contributors of this file:
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
 * @file nfc-emulate-forum-tag4.c
 * @brief Emulates a NFC Forum Tag Type 4 v2.0 (or v1.0) with a NDEF message
 */

/*
 * This implementation was written based on information provided by the
 * following documents:
 *
 * NFC Forum Type 4 Tag Operation
 *  Technical Specification
 *  NFCForum-TS-Type-4-Tag_1.0 - 2007-03-13
 *  NFCForum-TS-Type-4-Tag_2.0 - 2010-11-18
 */

// Notes & differences with nfc-emulate-tag:
// - This example only works with PN532 because it relies on
//   its internal handling of ISO14443-4 specificities.
// - Thanks to this internal handling & injection of WTX frames,
//   this example works on readers very strict on timing

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-emulation.h>

#include "nfc-utils.h"

static nfc_device *pnd;
static nfc_context *context;
static bool quiet_output = false;
// Version of the emulated type4 tag:
static int type4v = 2;

#define SYMBOL_PARAM_fISO14443_4_PICC   0x20

typedef enum { NONE, CC_FILE, NDEF_FILE } file;

struct nfcforum_tag4_ndef_data {
  uint8_t *ndef_file;
  size_t   ndef_file_len;
};

struct nfcforum_tag4_state_machine_data {
  file     current_file;
};

uint8_t nfcforum_capability_container[] = {
  0x00, 0x0F, /* CCLEN 15 bytes */
  0x20,       /* Mapping version 2.0, use option -1 to force v1.0 */
  0x00, 0x54, /* MLe Maximum R-ADPU data size */
// Notes:
//  - I (Romuald) don't know why Nokia 6212 Classic refuses the NDEF message if MLe is more than 0xFD (any suggests are welcome);
//  - ARYGON devices doesn't support extended frame sending, consequently these devices can't sent more than 0xFE bytes as APDU, so 0xFB APDU data bytes.
//  - I (Romuald) don't know why ARYGON device doesn't ACK when MLe > 0x54 (ARYGON frame length = 0xC2 (192 bytes))
  0x00, 0xFF, /* MLc Maximum C-ADPU data size */
  0x04,       /* T field of the NDEF File-Control TLV */
  0x06,       /* L field of the NDEF File-Control TLV */
  /* V field of the NDEF File-Control TLV */
  0xE1, 0x04, /* File identifier */
  0xFF, 0xFE, /* Maximum NDEF Size */
  0x00,       /* NDEF file read access condition */
  0x00,       /* NDEF file write access condition */
};

/* C-ADPU offsets */
#define CLA  0
#define INS  1
#define P1   2
#define P2   3
#define LC   4
#define DATA 5

#define ISO144434A_RATS 0xE0

static int
nfcforum_tag4_io(struct nfc_emulator *emulator, const uint8_t *data_in, const size_t data_in_len, uint8_t *data_out, const size_t data_out_len)
{
  int res = 0;

  struct nfcforum_tag4_ndef_data *ndef_data = (struct nfcforum_tag4_ndef_data *)(emulator->user_data);
  struct nfcforum_tag4_state_machine_data *state_machine_data = (struct nfcforum_tag4_state_machine_data *)(emulator->state_machine->data);

  if (data_in_len == 0) {
    // No input data, nothing to do
    return res;
  }

  // Show transmitted command
  if (!quiet_output) {
    printf("    In: ");
    print_hex(data_in, data_in_len);
  }

  if (data_in_len >= 4) {
    if (data_in[CLA] != 0x00)
      return -ENOTSUP;

#define ISO7816_SELECT         0xA4
#define ISO7816_READ_BINARY    0xB0
#define ISO7816_UPDATE_BINARY  0xD6

    switch (data_in[INS]) {
      case ISO7816_SELECT:

        switch (data_in[P1]) {
          case 0x00: /* Select by ID */
            if ((data_in[P2] | 0x0C) != 0x0C)
              return -ENOTSUP;

            const uint8_t ndef_capability_container[] = { 0xE1, 0x03 };
            const uint8_t ndef_file[] = { 0xE1, 0x04 };
            if ((data_in[LC] == sizeof(ndef_capability_container)) && (0 == memcmp(ndef_capability_container, data_in + DATA, data_in[LC]))) {
              memcpy(data_out, "\x90\x00", res = 2);
              state_machine_data->current_file = CC_FILE;
            } else if ((data_in[LC] == sizeof(ndef_file)) && (0 == memcmp(ndef_file, data_in + DATA, data_in[LC]))) {
              memcpy(data_out, "\x90\x00", res = 2);
              state_machine_data->current_file = NDEF_FILE;
            } else {
              memcpy(data_out, "\x6a\x00", res = 2);
              state_machine_data->current_file = NONE;
            }

            break;
          case 0x04: /* Select by name */
            if (data_in[P2] != 0x00)
              return -ENOTSUP;

            const uint8_t ndef_tag_application_name_v1[] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00 };
            const uint8_t ndef_tag_application_name_v2[] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
            if ((type4v == 1) && (data_in[LC] == sizeof(ndef_tag_application_name_v1)) && (0 == memcmp(ndef_tag_application_name_v1, data_in + DATA, data_in[LC])))
              memcpy(data_out, "\x90\x00", res = 2);
            else if ((type4v == 2) && (data_in[LC] == sizeof(ndef_tag_application_name_v2)) && (0 == memcmp(ndef_tag_application_name_v2, data_in + DATA, data_in[LC])))
              memcpy(data_out, "\x90\x00", res = 2);
            else
              memcpy(data_out, "\x6a\x82", res = 2);

            break;
          default:
            return -ENOTSUP;
        }

        break;
      case ISO7816_READ_BINARY:
        if ((size_t)(data_in[LC] + 2) > data_out_len) {
          return -ENOSPC;
        }
        switch (state_machine_data->current_file) {
          case NONE:
            memcpy(data_out, "\x6a\x82", res = 2);
            break;
          case CC_FILE:
            memcpy(data_out, nfcforum_capability_container + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
            memcpy(data_out + data_in[LC], "\x90\x00", 2);
            res = data_in[LC] + 2;
            break;
          case NDEF_FILE:
            memcpy(data_out, ndef_data->ndef_file + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
            memcpy(data_out + data_in[LC], "\x90\x00", 2);
            res = data_in[LC] + 2;
            break;
        }
        break;

      case ISO7816_UPDATE_BINARY:
        memcpy(ndef_data->ndef_file + (data_in[P1] << 8) + data_in[P2], data_in + DATA, data_in[LC]);
        if ((data_in[P1] << 8) + data_in[P2] == 0) {
          ndef_data->ndef_file_len = (ndef_data->ndef_file[0] << 8) + ndef_data->ndef_file[1] + 2;
        }
        memcpy(data_out, "\x90\x00", res = 2);
        break;
      default: // Unknown
        if (!quiet_output) {
          printf("Unknown frame, emulated target abort.\n");
        }
        res = -ENOTSUP;
    }
  } else {
    res = -ENOTSUP;
  }

  // Show transmitted command
  if (!quiet_output) {
    if (res < 0) {
      ERR("%s (%d)", strerror(-res), -res);
    } else {
      printf("    Out: ");
      print_hex(data_out, res);
    }
  }
  return res;
}

static void stop_emulation(int sig)
{
  (void) sig;
  if (pnd != NULL) {
    nfc_abort_command(pnd);
  } else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

static int
ndef_message_load(char *filename, struct nfcforum_tag4_ndef_data *tag_data)
{
  struct stat sb;
  if (stat(filename, &sb) < 0) {
    printf("file not found or not accessible '%s'", filename);
    return -1;
  }

  /* Check file size */
  if (sb.st_size > 0xFFFF) {
    printf("file size too large '%s'", filename);
    return -1;
  }

  tag_data->ndef_file_len = sb.st_size + 2;

  tag_data->ndef_file[0] = (uint8_t)(sb.st_size >> 8);
  tag_data->ndef_file[1] = (uint8_t)(sb.st_size);

  FILE *F;
  if (!(F = fopen(filename, "r"))) {
    printf("fopen (%s, \"r\")", filename);
    return -1;
  }

  if (1 != fread(tag_data->ndef_file + 2, sb.st_size, 1, F)) {
    printf("Can't read from %s", filename);
    fclose(F);
    return -1;
  }

  fclose(F);
  return sb.st_size;
}

static int
ndef_message_save(char *filename, struct nfcforum_tag4_ndef_data *tag_data)
{
  FILE *F;
  if (!(F = fopen(filename, "w"))) {
    printf("fopen (%s, w)", filename);
    return -1;
  }

  if (1 != fwrite(tag_data->ndef_file + 2, tag_data->ndef_file_len - 2, 1, F)) {
    printf("fwrite (%d)", (int) tag_data->ndef_file_len - 2);
    fclose(F);
    return -1;
  }

  fclose(F);
  return tag_data->ndef_file_len - 2;
}

static void
usage(char *progname)
{
  fprintf(stderr, "usage: %s [-1] [infile [outfile]]\n", progname);
  fprintf(stderr, "      -1: force Tag Type 4 v1.0 (default is v2.0)\n");
}

int
main(int argc, char *argv[])
{
  int options = 0;
  nfc_target nt = {
    .nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_UNDEFINED, // Will be updated by nfc_target_init()
    },
    .nti = {
      .nai = {
        .abtAtqa = { 0x00, 0x04 },
        .abtUid = { 0x08, 0x00, 0xb0, 0x0b },
        .szUidLen = 4,
        .btSak = 0x20,
        .abtAts = { 0x75, 0x33, 0x92, 0x03 }, /* Not used by PN532 */
        .szAtsLen = 4,
      },
    },
  };

  uint8_t ndef_file[0xfffe] = {
    0x00, 33,
    0xd1, 0x02, 0x1c, 0x53, 0x70, 0x91, 0x01, 0x09, 0x54, 0x02,
    0x65, 0x6e, 0x4c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x51, 0x01,
    0x0b, 0x55, 0x03, 0x6c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x2e,
    0x6f, 0x72, 0x67
  };

  struct nfcforum_tag4_ndef_data nfcforum_tag4_data = {
    .ndef_file = ndef_file,
    .ndef_file_len = ndef_file[1] + 2,
  };

  struct nfcforum_tag4_state_machine_data state_machine_data = {
    .current_file = NONE,
  };

  struct nfc_emulation_state_machine state_machine = {
    .io   = nfcforum_tag4_io,
    .data = &state_machine_data,
  };

  struct nfc_emulator emulator = {
    .target = &nt,
    .state_machine = &state_machine,
    .user_data = &nfcforum_tag4_data,
  };

  if ((argc > (1 + options)) && (0 == strcmp("-h", argv[1 + options]))) {
    usage(argv[0]);
    exit(EXIT_SUCCESS);
  }

  if ((argc > (1 + options)) && (0 == strcmp("-1", argv[1 + options]))) {
    type4v = 1;
    nfcforum_capability_container[2] = 0x10;
    options += 1;
  }

  if (argc > (3 + options)) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // If some file is provided load it
  if (argc >= (2 + options)) {
    if (ndef_message_load(argv[1 + options], &nfcforum_tag4_data) < 0) {
      printf("Can't load NDEF file '%s'", argv[1 + options]);
      exit(EXIT_FAILURE);
    }
  }

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)\n");
    exit(EXIT_FAILURE);
  }

  // Try to open the NFC reader
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("Unable to open NFC device");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, stop_emulation);

  printf("NFC device: %s opened\n", nfc_device_get_name(pnd));
  printf("Emulating NDEF tag now, please touch it with a second NFC device\n");

  if (0 != nfc_emulate_target(pnd, &emulator, 0)) {  // contains already nfc_target_init() call
    nfc_perror(pnd, "nfc_emulate_target");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (argc == (3 + options)) {
    if (ndef_message_save(argv[2 + options], &nfcforum_tag4_data) < 0) {
      printf("Can't save NDEF file '%s'", argv[2 + options]);
      nfc_close(pnd);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
