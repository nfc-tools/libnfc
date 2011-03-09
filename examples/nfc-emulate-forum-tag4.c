/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2010, Roel Verdult, Romuald Conty
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
 * @brief Emulates a NFC Forum Tag Type 4 with a NDEF message
 */

// Notes & differences with nfc-emulate-tag:
// - This example only works with PN532 because it relies on
//   its internal handling of ISO14443-4 specificities.
// - Thanks to this internal handling & injection of WTX frames,
//   this example works on readers very strict on timing
// - This example expects a hardcoded list of commands and
//   more precisely the commands sent by a Nokia NFC when
//   discovering a NFC-Forum tag type4:
//   * Anticoll & RATS
//   * App Select by name "e103e103e103"
//   * App Select by name "e103e103e103"
//   * App Select by name "D2760000850100"
//   * Select CC
//   * ReadBinary CC
//   * Select NDEF
//   * Read first 2 NDEF bytes
//   * Read remaining of NDEF file

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <sys/endian.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

#define MAX_FRAME_LEN 264

static byte_t abtRx[MAX_FRAME_LEN];
static size_t szRx = sizeof(abtRx);
static nfc_device_t *pnd;
static bool quiet_output = false;

#define SYMBOL_PARAM_fISO14443_4_PICC   0x20

struct nfc_emulator;

struct nfc_emulator {
  nfc_target_t *target;
  void *data;
  int (*io) (struct nfc_emulator *emulator, const byte_t *data_in, const size_t data_in_len, byte_t *data_out, const size_t data_out_len);
};

typedef enum { NONE, CC_FILE, NDEF_FILE } file;

struct nfcforum_tag4 {
  uint8_t *ndef_file;
  size_t   ndef_file_len;
  file     current_file;
};

uint8_t nfcforum_capability_container[] = {
  0x00, 0x0F, /* CCLEN 15 bytes */
  0x10,       /* Mapping version 1.0 */
  0x00, 0xFF, /* MLe Maximum R-ADPU data size */
  0x00, 0xFF, /* MLc Maximum C-ADPU data size */
  0x04,       /* T field of the NDEF File-Control TLV */
  0x06,       /* L field of the NDEF File-Control TLV */
              /* V field of the NDEF File-Control TLV */
  0xE1, 0x04, /* File identifier */
  0x00, 0xFE, /* Maximum NDEF Size */
  0x00,       /* NDEF file read access condition */
  /* TODO Add write support */
  0xFF,       /* NDEF file write access condition */
};

uint8_t nfcforum_capability_container_2_0[] = {
  0x00, 0x0F, /* CCLEN 15 bytes */
  0x20,       /* Mapping version 2.0 */
  0x00, 0xFF, /* MLe Maximum R-ADPU data size */
  0x00, 0xFF, /* MLc Maximum C-ADPU data size */
  0x04,       /* T field of the NDEF File-Control TLV */
  0x06,       /* L field of the NDEF File-Control TLV */
              /* V field of the NDEF File-Control TLV */
  0xE1, 0x04, /* File identifier */
  0x00, 0xFE, /* Maximum NDEF Size */
  0x00,       /* NDEF file read access condition */
  /* TODO Add write support */
  0xFF,       /* NDEF file write access condition */
};


/* C-ADPU offsets */
#define CLA  0
#define INS  1
#define P1   2
#define P2   3
#define LC   4
#define DATA 5

#define ISO144434A_RATS 0xE0

int
nfcforum_tag4_io (struct nfc_emulator *emulator, const byte_t *data_in, const size_t data_in_len, byte_t *data_out, const size_t data_out_len)
{
  (void) emulator;
  (void) data_out_len;
  int res = 0;

  struct nfcforum_tag4 *data = (struct nfcforum_tag4 *)(emulator->data);

  // Show transmitted command
  if (!quiet_output) {
    printf ("    In: ");
    print_hex (data_in, data_in_len);
  }

  /*
   * The PN532 already handle RATS
   */
  if ((data_in_len == 2) && (data_in[0] == ISO144434A_RATS))
      return res;

  if(data_in_len >= 4) {
    if (data_in[CLA] != 0x00)
      return -ENOTSUP;

#define ISO7816_SELECT         0xA4
#define ISO7816_READ_BINARY    0xB0
#define ISO7816_UPDATE_BINARY  0xD6

    switch(data_in[INS]) {
    case ISO7816_SELECT:

      switch (data_in[P1]) {
      case 0x00: /* Select by ID */
        if ((data_in[P2] | 0x0C) != 0x0C)
          return -ENOTSUP;

        const uint8_t ndef_capability_container[] = { 0xE1, 0x03 };
        const uint8_t ndef_file[] = { 0xE1, 0x04 };
        if ((data_in[LC] == sizeof (ndef_capability_container)) && (0 == memcmp (ndef_capability_container, data_in + DATA, data_in[LC]))) {
          memcpy (data_out, "\x90\x00", res = 2);
          data->current_file = CC_FILE;
        } else if ((data_in[LC] == sizeof (ndef_file)) && (0 == memcmp (ndef_file, data_in + DATA, data_in[LC]))) {
          memcpy (data_out, "\x90\x00", res = 2);
          data->current_file = NDEF_FILE;
        } else {
          memcpy (data_out, "\x6a\x00", res = 2);
          data->current_file = NONE;
        }

        break;
      case 0x04: /* Select by name */
        if (data_in[P2] != 0x00)
          return -ENOTSUP;

        const uint8_t ndef_tag_application_name[] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00 };
        if ((data_in[LC] == sizeof (ndef_tag_application_name)) && (0 == memcmp (ndef_tag_application_name, data_in + DATA, data_in[LC])))
          memcpy (data_out, "\x90\x00", res = 2);
        else
          memcpy (data_out, "\x6a\x82", res = 2);

        break;
      default:
        return -ENOTSUP;
      }


      break;
    case ISO7816_READ_BINARY:
      if (data_in[LC] + 2 > data_out_len) {
        return -ENOSPC;
      }
      switch (data->current_file) {
      case NONE:
        memcpy (data_out, "\x6a\x82", res = 2);
        break;
      case CC_FILE:
        memcpy (data_out, nfcforum_capability_container + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
        memcpy (data_out + data_in[LC], "\x90\x00", 2);
        res = data_in[LC] + 2;
        break;
      case NDEF_FILE:
        memcpy (data_out, data->ndef_file + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
        memcpy (data_out + data_in[LC], "\x90\x00", 2);
        res = data_in[LC] + 2;
        break;
      }
      break;

    case ISO7816_UPDATE_BINARY:
      return -ENOTSUP;
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
    printf ("    Out: ");
    if (res < 0)
      printf ("No data (returning with an error %d)\n", res);
    else
      print_hex (data_out, res);
  }
  return res;
}


bool
nfc_emulate_target (nfc_device_t* pnd, struct nfc_emulator *emulator)
{
  size_t szRx;
  byte_t abtTx[MAX_FRAME_LEN];
  int res = 0;

  if (!nfc_target_init (pnd, emulator->target, abtRx, &szRx)) {
    nfc_perror (pnd, "nfc_target_init");
    return false;
  }

  while (res >= 0) {
    res = emulator->io (emulator, abtRx, szRx, abtTx, sizeof (abtTx));
    if (res > 0) {
      if (!nfc_target_send_bytes(pnd, abtTx, res)) {
        nfc_perror (pnd, "nfc_target_send_bytes");
        return false;
      }
    }
    if (res >= 0) {
      if (!nfc_target_receive_bytes(pnd, abtRx, &szRx)) {
        nfc_perror (pnd, "nfc_target_receive_bytes");
        return false;
      }
    }
  }
  return true;
}

void stop_emulation (int sig)
{
  (void) sig;
  if (pnd)
    nfc_abort_command (pnd);
  else
    exit (EXIT_FAILURE);
}

bool send_bytes (const byte_t * pbtTx, const size_t szTx)
{
  // Show transmitted command
  if (!quiet_output) {
    printf ("Sent data: ");
    print_hex (pbtTx, szTx);
  }

  // Transmit the command bytes
  if (!nfc_target_send_bytes(pnd, pbtTx, szTx)) {
    nfc_perror (pnd, "nfc_target_send_bytes");
    exit(EXIT_FAILURE);
  }
  // Succesful transfer
  return true;
}

bool receive_bytes (void)
{
  if (!nfc_target_receive_bytes(pnd,abtRx,&szRx)) {
    nfc_perror (pnd, "nfc_target_receive_bytes");
    exit(EXIT_FAILURE);
  }

  // Show received answer
  if (!quiet_output) {
    printf ("Received data: ");
    print_hex (abtRx, szRx);
  }
  // Succesful transfer
  return true;
}

int
main (void)
{
  // Try to open the NFC reader
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    ERR("Unable to connect to NFC device");
    exit (EXIT_FAILURE);
  }

  signal (SIGINT, stop_emulation);

  printf ("Connected to NFC device: %s\n", pnd->acName);
  printf ("Emulating NDEF tag now, please touch it with a second NFC device\n");

  nfc_target_t nt = {
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

  uint8_t ndef_file[] = {
    0x00, 33,
    0xd1, 0x02, 0x1c, 0x53, 0x70, 0x91, 0x01, 0x09, 0x54, 0x02,
    0x65, 0x6e, 0x4c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x51, 0x01,
    0x0b, 0x55, 0x03, 0x6c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x2e,
    0x6f, 0x72, 0x67
  };

  struct nfcforum_tag4 nfcforum_tag4_data = {
    .ndef_file = ndef_file,
    .ndef_file_len = sizeof (ndef_file),
    .current_file = NONE,
  };

  struct nfc_emulator emulator = {
    .target = &nt,
    .io   = nfcforum_tag4_io,
    .data = &nfcforum_tag4_data,
  };

  nfc_emulate_target (pnd, &emulator);

  nfc_disconnect(pnd);
  exit (EXIT_SUCCESS);
}
