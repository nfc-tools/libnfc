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
 * @file nfc-relay-picc.c
 * @brief Relay example using two PN532 devices.
 */

// Notes & differences with nfc-relay:
// - This example only works with PN532 because it relies on
//   its internal handling of ISO14443-4 specificities.
// - Thanks to this internal handling & injection of WTX frames,
//   this example works on readers very strict on timing

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>

#include <nfc/nfc.h>

#include "nfc-utils.h"

#define MAX_FRAME_LEN 264
#define MAX_DEVICE_COUNT 2

static uint8_t abtCapdu[MAX_FRAME_LEN];
static size_t szCapduLen;
static uint8_t abtRapdu[MAX_FRAME_LEN];
static size_t szRapduLen;
static nfc_device *pndInitiator;
static nfc_device *pndTarget;
static bool quitting = false;
static bool quiet_output = false;
static bool initiator_only_mode = false;
static bool target_only_mode = false;
static bool swap_devices = false;
static int waiting_time = 0;
FILE *fd3;
FILE *fd4;

static void
intr_hdlr(int sig)
{
  (void) sig;
  printf("\nQuitting...\n");
  printf("Please send a last command to the emulator to quit properly.\n");
  quitting = true;
  return;
}

static void
print_usage(char *argv[])
{
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tHelp. Print this message.\n");
  printf("\t-q\tQuiet mode. Suppress printing of relayed data (improves timing).\n");
  printf("\t-t\tTarget mode only (the one on reader side). Data expected from FD3 to FD4.\n");
  printf("\t-i\tInitiator mode only (the one on tag side). Data expected from FD3 to FD4.\n");
  printf("\t-n N\tAdds a waiting time of N seconds (integer) in the relay to mimic long distance.\n");
}

static int print_hex_fd4(const uint8_t *pbtData, const size_t szBytes, const char *pchPrefix)
{
  size_t  szPos;
  if (szBytes > MAX_FRAME_LEN) {
    return -1;
  }
  if (fprintf(fd4, "#%s %04" PRIxPTR ": ", pchPrefix, szBytes) < 0) {
    return -1;
  }

  for (szPos = 0; szPos < szBytes; szPos++) {
    if (fprintf(fd4, "%02x ", pbtData[szPos]) < 0) {
      return -1;
    }
  }
  if (fprintf(fd4, "\n") < 0) {
    return -1;
  }
  fflush(fd4);
  return 0;
}

static int scan_hex_fd3(uint8_t *pbtData, size_t *pszBytes, const char *pchPrefix)
{
  size_t  szPos;
  unsigned int uiBytes;
  unsigned int uiData;
  char pchScan[256];
  int c;
  // Look for our next sync marker
  while ((c = fgetc(fd3)) != '#') {
    if (c == EOF) {
      return -1;
    }
  }
  strncpy(pchScan, pchPrefix, 250);
  pchScan[sizeof(pchScan) - 1] = '\0';
  strcat(pchScan, " %04x:");
  if (fscanf(fd3, pchScan, &uiBytes) < 1) {
    return -1;
  }
  *pszBytes = uiBytes;
  if (*pszBytes > MAX_FRAME_LEN) {
    return -1;
  }
  for (szPos = 0; szPos < *pszBytes; szPos++) {
    if (fscanf(fd3, "%02x", &uiData) < 1) {
      return -1;
    }
    pbtData[szPos] = uiData;
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  int     arg;
  const char *acLibnfcVersion = nfc_version();
  nfc_target ntRealTarget;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      exit(EXIT_SUCCESS);
    } else if (0 == strcmp(argv[arg], "-q")) {
      quiet_output = true;
    } else if (0 == strcmp(argv[arg], "-t")) {
      printf("INFO: %s\n", "Target mode only.");
      initiator_only_mode = false;
      target_only_mode = true;
    } else if (0 == strcmp(argv[arg], "-i")) {
      printf("INFO: %s\n", "Initiator mode only.");
      initiator_only_mode = true;
      target_only_mode = false;
    } else if (0 == strcmp(argv[arg], "-s")) {
      printf("INFO: %s\n", "Swapping devices.");
      swap_devices = true;
    } else if (0 == strcmp(argv[arg], "-n")) {
      if (++arg == argc || (sscanf(argv[arg], "%10i", &waiting_time) < 1)) {
        ERR("Missing or wrong waiting time value: %s.", argv[arg]);
        print_usage(argv);
        exit(EXIT_FAILURE);
      }
      printf("Waiting time: %i secs.\n", waiting_time);
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      exit(EXIT_FAILURE);
    }
  }

  // Display libnfc version
  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

#ifdef WIN32
  signal(SIGINT, (void (__cdecl *)(int)) intr_hdlr);
#else
  signal(SIGINT, intr_hdlr);
#endif

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  // List available devices
  size_t szFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

  if (initiator_only_mode || target_only_mode) {
    if (szFound < 1) {
      ERR("No device found");
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
    if ((fd3 = fdopen(3, "r")) == NULL) {
      ERR("Could not open file descriptor 3");
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
    if ((fd4 = fdopen(4, "r")) == NULL) {
      ERR("Could not open file descriptor 4");
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
  } else {
    if (szFound < 2) {
      ERR("%" PRIdPTR " device found but two opened devices are needed to relay NFC.", szFound);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
  }

  if (!target_only_mode) {
    // Try to open the NFC reader used as initiator
    // Little hack to allow using initiator no matter if
    // there is already a target used locally or not on the same machine:
    // if there is more than one readers opened we open the second reader
    // (we hope they're always detected in the same order)
    if ((szFound == 1) || swap_devices) {
      pndInitiator = nfc_open(context, connstrings[0]);
    } else {
      pndInitiator = nfc_open(context, connstrings[1]);
    }

    if (pndInitiator == NULL) {
      printf("Error opening NFC reader\n");
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }

    printf("NFC reader device: %s opened\n", nfc_device_get_name(pndInitiator));

    if (nfc_initiator_init(pndInitiator) < 0) {
      printf("Error: fail initializing initiator\n");
      nfc_close(pndInitiator);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }

    // Try to find a ISO 14443-4A tag
    nfc_modulation nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_106,
    };
    if (nfc_initiator_select_passive_target(pndInitiator, nm, NULL, 0, &ntRealTarget) <= 0) {
      printf("Error: no tag was found\n");
      nfc_close(pndInitiator);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }

    printf("Found tag:\n");
    print_nfc_target(&ntRealTarget, false);
    if (initiator_only_mode) {
      if (print_hex_fd4(ntRealTarget.nti.nai.abtUid, ntRealTarget.nti.nai.szUidLen, "UID") < 0) {
        fprintf(stderr, "Error while printing UID to FD4\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(ntRealTarget.nti.nai.abtAtqa, 2, "ATQA") < 0) {
        fprintf(stderr, "Error while printing ATQA to FD4\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(&(ntRealTarget.nti.nai.btSak), 1, "SAK") < 0) {
        fprintf(stderr, "Error while printing SAK to FD4\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(ntRealTarget.nti.nai.abtAts, ntRealTarget.nti.nai.szAtsLen, "ATS") < 0) {
        fprintf(stderr, "Error while printing ATS to FD4\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
    }
  }
  if (initiator_only_mode) {
    printf("Hint: tag <---> *INITIATOR* (relay) <-FD3/FD4-> target (relay) <---> original reader\n\n");
  } else if (target_only_mode) {
    printf("Hint: tag <---> initiator (relay) <-FD3/FD4-> *TARGET* (relay) <---> original reader\n\n");
  } else {
    printf("Hint: tag <---> initiator (relay) <---> target (relay) <---> original reader\n\n");
  }
  if (!initiator_only_mode) {
    nfc_target ntEmulatedTarget = {
      .nm = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
      },
    };
    if (target_only_mode) {
      size_t foo;
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtUid, &(ntEmulatedTarget.nti.nai.szUidLen), "UID") < 0) {
        fprintf(stderr, "Error while scanning UID from FD3\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtAtqa, &foo, "ATQA") < 0) {
        fprintf(stderr, "Error while scanning ATQA from FD3\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(&(ntEmulatedTarget.nti.nai.btSak), &foo, "SAK") < 0) {
        fprintf(stderr, "Error while scanning SAK from FD3\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtAts, &(ntEmulatedTarget.nti.nai.szAtsLen), "ATS") < 0) {
        fprintf(stderr, "Error while scanning ATS from FD3\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
    } else {
      ntEmulatedTarget.nti = ntRealTarget.nti;
    }
    // We can only emulate a short UID, so fix length & ATQA bit:
    ntEmulatedTarget.nti.nai.szUidLen = 4;
    ntEmulatedTarget.nti.nai.abtAtqa[1] &= (0xFF - 0x40);
    // First byte of UID is always automatically replaced by 0x08 in this mode anyway
    ntEmulatedTarget.nti.nai.abtUid[0] = 0x08;
    // ATS is always automatically replaced by PN532, we've no control on it:
    // ATS = (05) 75 33 92 03
    //       (TL) T0 TA TB TC
    //             |  |  |  +-- CID supported, NAD supported
    //             |  |  +----- FWI=9 SFGI=2 => FWT=154ms, SFGT=1.21ms
    //             |  +-------- DR=2,4 DS=2,4 => supports 106, 212 & 424bps in both directions
    //             +----------- TA,TB,TC, FSCI=5 => FSC=64
    // It seems hazardous to tell we support NAD if the tag doesn't support NAD but I don't know how to disable it
    // PC/SC pseudo-ATR = 3B 80 80 01 01 if there is no historical bytes

    // Creates ATS and copy max 48 bytes of Tk:
    uint8_t *pbtTk;
    size_t szTk;
    pbtTk = iso14443a_locate_historical_bytes(ntEmulatedTarget.nti.nai.abtAts, ntEmulatedTarget.nti.nai.szAtsLen, &szTk);
    szTk = (szTk > 48) ? 48 : szTk;
    uint8_t pbtTkt[48];
    memcpy(pbtTkt, pbtTk, szTk);
    ntEmulatedTarget.nti.nai.abtAts[0] = 0x75;
    ntEmulatedTarget.nti.nai.abtAts[1] = 0x33;
    ntEmulatedTarget.nti.nai.abtAts[2] = 0x92;
    ntEmulatedTarget.nti.nai.abtAts[3] = 0x03;
    ntEmulatedTarget.nti.nai.szAtsLen = 4 + szTk;
    memcpy(&(ntEmulatedTarget.nti.nai.abtAts[4]), pbtTkt, szTk);

    printf("We will emulate:\n");
    print_nfc_target(&ntEmulatedTarget, false);

    // Try to open the NFC emulator device
    if (swap_devices) {
      pndTarget = nfc_open(context, connstrings[1]);
    } else {
      pndTarget = nfc_open(context, connstrings[0]);
    }
    if (pndTarget == NULL) {
      printf("Error opening NFC emulator device\n");
      if (!target_only_mode) {
        nfc_close(pndInitiator);
      }
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }

    printf("NFC emulator device: %s opened\n", nfc_device_get_name(pndTarget));
    if (nfc_target_init(pndTarget, &ntEmulatedTarget, abtCapdu, sizeof(abtCapdu), 0) < 0) {
      ERR("%s", "Initialization of NFC emulator failed");
      if (!target_only_mode) {
        nfc_close(pndInitiator);
      }
      nfc_close(pndTarget);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
    printf("%s\n", "Done, relaying frames now!");
  }

  while (!quitting) {
    bool ret;
    int res = 0;
    if (!initiator_only_mode) {
      // Receive external reader command through target
      if ((res = nfc_target_receive_bytes(pndTarget, abtCapdu, sizeof(abtCapdu), 0)) < 0) {
        nfc_perror(pndTarget, "nfc_target_receive_bytes");
        if (!target_only_mode) {
          nfc_close(pndInitiator);
        }
        nfc_close(pndTarget);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      szCapduLen = (size_t) res;
      if (target_only_mode) {
        if (print_hex_fd4(abtCapdu, szCapduLen, "C-APDU") < 0) {
          fprintf(stderr, "Error while printing C-APDU to FD4\n");
          nfc_close(pndTarget);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
      }
    } else {
      if (scan_hex_fd3(abtCapdu, &szCapduLen, "C-APDU") < 0) {
        fprintf(stderr, "Error while scanning C-APDU from FD3\n");
        nfc_close(pndInitiator);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
    }
    // Show transmitted response
    if (!quiet_output) {
      printf("Forwarding C-APDU: ");
      print_hex(abtCapdu, szCapduLen);
    }

    if (!target_only_mode) {
      // Forward the frame to the original tag
      if ((res = nfc_initiator_transceive_bytes(pndInitiator, abtCapdu, szCapduLen, abtRapdu, sizeof(abtRapdu), -1)) < 0) {
        ret = false;
      } else {
        szRapduLen = (size_t) res;
        ret = true;
      }
    } else {
      if (scan_hex_fd3(abtRapdu, &szRapduLen, "R-APDU") < 0) {
        fprintf(stderr, "Error while scanning R-APDU from FD3\n");
        nfc_close(pndTarget);
        nfc_exit(context);
        exit(EXIT_FAILURE);
      }
      ret = true;
    }
    if (ret) {
      // Redirect the answer back to the external reader
      if (waiting_time > 0) {
        if (!quiet_output) {
          printf("Waiting %is to simulate longer relay...\n", waiting_time);
        }
        sleep(waiting_time);
      }
      // Show transmitted response
      if (!quiet_output) {
        printf("Forwarding R-APDU: ");
        print_hex(abtRapdu, szRapduLen);
      }
      if (!initiator_only_mode) {
        // Transmit the response bytes
        if (nfc_target_send_bytes(pndTarget, abtRapdu, szRapduLen, 0) < 0) {
          nfc_perror(pndTarget, "nfc_target_send_bytes");
          if (!target_only_mode) {
            nfc_close(pndInitiator);
          }
          if (!initiator_only_mode) {
            nfc_close(pndTarget);
            nfc_exit(context);
          }
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
      } else {
        if (print_hex_fd4(abtRapdu, szRapduLen, "R-APDU") < 0) {
          fprintf(stderr, "Error while printing R-APDU to FD4\n");
          nfc_close(pndInitiator);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  if (!target_only_mode) {
    nfc_close(pndInitiator);
  }
  if (!initiator_only_mode) {
    nfc_close(pndTarget);
  }
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}

