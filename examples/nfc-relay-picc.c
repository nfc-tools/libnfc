/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Romuald Conty
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
//#include <stddef.h>

#include <nfc/nfc.h>

#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#ifndef _WIN32
// Needed by sleep() under Unix
#  include <unistd.h>
#  define sleep sleep
#  define SUSP_TIME 1           // secs.
#else
// Needed by Sleep() under Windows
#  include <winbase.h>
#  define sleep Sleep
#  define SUSP_TIME 1000        // msecs.
#endif

#define MAX_FRAME_LEN 264
#define MAX_DEVICE_COUNT 2

static byte_t abtCapdu[MAX_FRAME_LEN];
static size_t szCapduLen;
static byte_t abtRapdu[MAX_FRAME_LEN];
static size_t szRapduLen;
static nfc_device_t *pndInitiator;
static nfc_device_t *pndTarget;
static bool quitting = false;
static bool quiet_output = false;
static bool initiator_only_mode = false;
static bool target_only_mode = false;
static int waiting_time = 0;
FILE * fd3;
FILE * fd4;

#define SYMBOL_PARAM_fISO14443_4_PICC   0x20

void
intr_hdlr (void)
{
  printf ("\nQuitting...\n");
  printf ("Please send a last command to the emulator to quit properly.\n");
  quitting = true;
  return;
}

void
print_usage (char *argv[])
{
  printf ("Usage: %s [OPTIONS]\n", argv[0]);
  printf ("Options:\n");
  printf ("\t-h\tHelp. Print this message.\n");
  printf ("\t-q\tQuiet mode. Suppress printing of relayed data (improves timing).\n");
  printf ("\t-t\tTarget mode only (the one on reader side). Data expected from FD3 to FD4.\n");
  printf ("\t-i\tInitiator mode only (the one on tag side). Data expected from FD3 to FD4.\n");
  printf ("\t-n N\tAdds a waiting time of N seconds (integer) in the relay to mimic long distance.\n");
}

bool print_hex_fd4 (const byte_t * pbtData, const size_t szBytes, const char * pchPrefix)
{
  size_t  szPos;
  if (szBytes > MAX_FRAME_LEN) {
    return EXIT_FAILURE;
  }
  if (fprintf (fd4, "#%s %04x: ", pchPrefix, szBytes)<0) {
    return EXIT_FAILURE;
  }

  for (szPos = 0; szPos < szBytes; szPos++) {
    if (fprintf (fd4, "%02x ", pbtData[szPos])<0) {
      return EXIT_FAILURE;
    }
  }
  if (fprintf (fd4, "\n")<0) {
    return EXIT_FAILURE;
  }
  fflush(fd4);
  return EXIT_SUCCESS;
}

bool scan_hex_fd3 (byte_t *pbtData, size_t *pszBytes, const char * pchPrefix)
{
  size_t  szPos;
  unsigned int uiBytes;
  unsigned int uiData;
  char pchScan[256];
  int c;
  // Look for our next sync marker
  while ( (c=fgetc(fd3)) != '#') {
    if (c == EOF) {
      return EXIT_FAILURE;
    }
  }
  strncpy(pchScan, pchPrefix, 250);
  strcat(pchScan, " %04x:");
  if (fscanf (fd3, pchScan, &uiBytes)<1) {
    return EXIT_FAILURE;
  }
  *pszBytes=uiBytes;
  if (*pszBytes > MAX_FRAME_LEN) {
    return EXIT_FAILURE;
  }
  for (szPos = 0; szPos < *pszBytes; szPos++) {
    if (fscanf (fd3, "%02x", &uiData)<1) {
      return EXIT_FAILURE;
    }
    pbtData[szPos]=uiData;
  }
  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  int     arg;
  size_t  szFound;
  nfc_device_desc_t *pnddDevices;
  const char *acLibnfcVersion = nfc_version ();
  nfc_target_t ntRealTarget;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {
    if (0 == strcmp (argv[arg], "-h")) {
      print_usage (argv);
      return EXIT_SUCCESS;
    } else if (0 == strcmp (argv[arg], "-q")) {
      INFO ("%s", "Quiet mode.");
      quiet_output = true;
    } else if (0 == strcmp (argv[arg], "-t")) {
      INFO ("%s", "Target mode only.");
      initiator_only_mode = false;
      target_only_mode = true;
    } else if (0 == strcmp (argv[arg], "-i")) {
      INFO ("%s", "Initiator mode only.");
      initiator_only_mode = true;
      target_only_mode = false;
    } else if (0 == strcmp (argv[arg], "-n")) {
      if (++arg==argc || (sscanf(argv[arg], "%i", &waiting_time)<1)) {
        ERR ("Missing or wrong waiting time value: %s.", argv[arg]);
        print_usage (argv);
        return EXIT_FAILURE;
      }
      INFO ("Waiting time: %i secs.", waiting_time);
    } else {
      ERR ("%s is not supported option.", argv[arg]);
      print_usage (argv);
      return EXIT_FAILURE;
    }
  }

  // Display libnfc version
  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

#ifdef WIN32
  signal (SIGINT, (void (__cdecl *) (int)) intr_hdlr);
#else
  signal (SIGINT, (void (*)()) intr_hdlr);
#endif

  // Allocate memory to put the result of available devices listing
  if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
    fprintf (stderr, "malloc() failed\n");
    return EXIT_FAILURE;
  }
  // List available devices
  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (initiator_only_mode || target_only_mode) {
    if (szFound < 1) {
      ERR ("No device found");
      return EXIT_FAILURE;
    }
    fd3 = fdopen(3, "r");
    fd4 = fdopen(4, "w");
  }
  else {
    if (szFound < 2) {
      ERR ("%zd device found but two connected devices are needed to relay NFC.", szFound);
      return EXIT_FAILURE;
    }
  }

  if (!target_only_mode) {
    // Try to open the NFC reader used as initiator
    // Little hack to allow using initiator no matter if
    // there is already a target used locally or not on the same machine:
    // if there is more than one readers connected we connect to the second reader
    // (we hope they're always detected in the same order)
    if (szFound == 1) {
      pndInitiator = nfc_connect (&(pnddDevices[0]));
    } else {
      pndInitiator = nfc_connect (&(pnddDevices[1]));
    }

    printf ("Connected to the NFC reader device: %s\n", pndInitiator->acName);

    // Try to find a ISO 14443-4A tag
    nfc_modulation_t nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_106,
    };
    if (!nfc_initiator_select_passive_target (pndInitiator, nm, NULL, 0, &ntRealTarget)) {
      printf ("Error: no tag was found\n");
      nfc_disconnect (pndInitiator);
     exit (EXIT_FAILURE);
    }

    printf("Found tag:\n");
    print_nfc_iso14443a_info (ntRealTarget.nti.nai);
    if (initiator_only_mode) {
      if (print_hex_fd4(ntRealTarget.nti.nai.abtUid, ntRealTarget.nti.nai.szUidLen, "UID") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing UID to FD4\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(ntRealTarget.nti.nai.abtAtqa, 2, "ATQA") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing ATQA to FD4\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(&(ntRealTarget.nti.nai.btSak), 1, "SAK") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing SAK to FD4\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (print_hex_fd4(ntRealTarget.nti.nai.abtAts, ntRealTarget.nti.nai.szAtsLen, "ATS") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing ATS to FD4\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
    } 
  }
  if (initiator_only_mode) {
    printf ("Hint: tag <---> *INITIATOR* (relay) <-FD3/FD4-> target (relay) <---> original reader\n\n");
  } else if (target_only_mode) {
    printf ("Hint: tag <---> initiator (relay) <-FD3/FD4-> *TARGET* (relay) <---> original reader\n\n");
  } else {
    printf ("Hint: tag <---> initiator (relay) <---> target (relay) <---> original reader\n\n");
  }
  if (!initiator_only_mode) {
    nfc_target_t ntEmulatedTarget = {
      .nm.nmt = NMT_ISO14443A,
      .nm.nbr = NBR_106,
    };
    if (target_only_mode) {
      size_t foo;
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtUid, &(ntEmulatedTarget.nti.nai.szUidLen), "UID") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning UID from FD3\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtAtqa, &foo, "ATQA") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning ATQA from FD3\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(&(ntEmulatedTarget.nti.nai.btSak), &foo, "SAK") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning SAK from FD3\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
      if (scan_hex_fd3(ntEmulatedTarget.nti.nai.abtAts, &(ntEmulatedTarget.nti.nai.szAtsLen), "ATS") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning ATS from FD3\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
    } else {
      ntEmulatedTarget.nti = ntRealTarget.nti;
    }
    // We can only emulate a short UID, so fix length & ATQA bit:
    ntEmulatedTarget.nti.nai.szUidLen = 4;
    ntEmulatedTarget.nti.nai.abtAtqa[1] &= (0xFF-0x40);
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
    // PC/SC pseudo-ATR = 3B 80 80 01 01
    ntEmulatedTarget.nti.nai.abtAts[0] = 0x75;
    ntEmulatedTarget.nti.nai.abtAts[1] = 0x33;
    ntEmulatedTarget.nti.nai.abtAts[2] = 0x92;
    ntEmulatedTarget.nti.nai.abtAts[3] = 0x03;
    ntEmulatedTarget.nti.nai.szAtsLen = 4;
    //FIXME we could actually emulate also the historical bytes of the tag once libnfc API supports it...

    printf("We will emulate:\n");
    print_nfc_iso14443a_info (ntEmulatedTarget.nti.nai);
 
    // Try to open the NFC emulator device
    pndTarget = nfc_connect (&(pnddDevices[0]));
    if (pndTarget == NULL) {
      printf ("Error connecting NFC emulator device\n");
      if (!target_only_mode) {
        nfc_disconnect (pndInitiator);
      }
      return EXIT_FAILURE;
    }

    printf ("Connected to the NFC emulator device: %s\n", pndTarget->acName);

    if (!nfc_target_init (pndTarget, NTM_ISO14443_4_PICC_ONLY, &ntEmulatedTarget, abtCapdu, &szCapduLen)) {
      ERR ("%s", "Initialization of NFC emulator failed");
      if (!target_only_mode) {
        nfc_disconnect (pndInitiator);
      }
      nfc_disconnect (pndTarget);
      exit(EXIT_FAILURE);
    }
    printf ("%s\n", "Done, relaying frames now!");
  }


  while (!quitting) {
    bool ret;
    if (!initiator_only_mode) {
      // Receive external reader command through target
      if (!nfc_target_receive_bytes(pndTarget,abtCapdu,&szCapduLen)) {
        nfc_perror (pndTarget, "nfc_target_receive_bytes");
        if (!target_only_mode) {
          nfc_disconnect (pndInitiator);
        }
        nfc_disconnect (pndTarget);
        exit(EXIT_FAILURE);
      }
      if (target_only_mode) {
        if (print_hex_fd4(abtCapdu, szCapduLen, "C-APDU") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing C-APDU to FD4\n");
          nfc_disconnect (pndTarget);
          exit(EXIT_FAILURE);
        }
      }
    } else {
      if (scan_hex_fd3(abtCapdu, &szCapduLen, "C-APDU") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning C-APDU from FD3\n");
        nfc_disconnect (pndInitiator);
        exit(EXIT_FAILURE);
      }
    }
    // Show transmitted response
    if (!quiet_output) {
      printf ("Forwarding C-APDU: ");
      print_hex (abtCapdu, szCapduLen);
    }

    if (!target_only_mode) {
      // Forward the frame to the original tag
      ret = nfc_initiator_transceive_bytes
          (pndInitiator, abtCapdu, szCapduLen, abtRapdu, &szRapduLen);
    } else {
      if (scan_hex_fd3(abtRapdu, &szRapduLen, "R-APDU") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while scanning R-APDU from FD3\n");
        nfc_disconnect (pndTarget);
        exit(EXIT_FAILURE);
      }
      ret = true;
    }
    if (ret) {
      // Redirect the answer back to the external reader
      if (waiting_time > 0) {
        if (!quiet_output) {
          printf ("Waiting %is to simulate longer relay...\n", waiting_time);
        }
        sleep(waiting_time * SUSP_TIME);
      }
      // Show transmitted response
      if (!quiet_output) {
        printf ("Forwarding R-APDU: ");
        print_hex (abtRapdu, szRapduLen);
      }
      if (!initiator_only_mode) {
        // Transmit the response bytes
        if (!nfc_target_send_bytes(pndTarget, abtRapdu, szRapduLen)) {
          nfc_perror (pndTarget, "nfc_target_send_bytes");
          if (!target_only_mode) {
            nfc_disconnect (pndInitiator);
          }
          if (!initiator_only_mode) {
            nfc_disconnect (pndTarget);
          }
          exit(EXIT_FAILURE);
        }
      } else {
        if (print_hex_fd4(abtRapdu, szRapduLen, "R-APDU") != EXIT_SUCCESS) {
        fprintf (stderr, "Error while printing R-APDU to FD4\n");
          nfc_disconnect (pndInitiator);
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  if (!target_only_mode) {
    nfc_disconnect (pndInitiator);
  }
  if (!initiator_only_mode) {
    nfc_disconnect (pndTarget);
  }
  exit (EXIT_SUCCESS);
}

