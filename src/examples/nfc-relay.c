/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * 
 * 
 * @file nfc-relay.c
 * @brief
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <nfc.h>

#include "messages.h"
#include "bitutils.h"

static byte_t abtReaderRx[MAX_FRAME_LEN];
static byte_t abtReaderRxPar[MAX_FRAME_LEN];
static size_t szReaderRxBits;
static byte_t abtTagRx[MAX_FRAME_LEN];
static byte_t abtTagRxPar[MAX_FRAME_LEN];
static size_t szTagRxBits;
static dev_info* pdiReader;
static dev_info* pdiTag;
static bool quitting=false;

void intr_hdlr(void)
{
  printf("\nQuitting...\n");
  quitting=true;
  return;
}

void print_usage(char* argv[])
{
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tHelp. Print this message.\n");
  printf("\t-q\tQuiet mode. Suppress output of READER and EMULATOR data (improves timing).\n");
}

int main(int argc,char* argv[])
{
  int arg;
  bool quiet_output = false;

  // Get commandline options
  for (arg=1;arg<argc;arg++) {
    if (0 == strcmp(argv[arg], "-h")) {
      print_usage(argv);
      return 0;
    } else if (0 == strcmp(argv[arg], "-q")) {
      INFO("Quiet mode.");
      quiet_output = true;
    } else {
      ERR("%s is not supported option.", argv[arg]);
      print_usage(argv);
      return -1;
    }
  }

#ifdef WIN32
  signal(SIGINT, (void (__cdecl*)(int)) intr_hdlr);
#else
  signal(SIGINT, (void (*)()) intr_hdlr);
#endif

  // Try to open the NFC emulator device
  pdiTag = nfc_connect(NULL);
  if (pdiTag == INVALID_DEVICE_INFO)
  {
    printf("Error connecting NFC emulator device\n");
    return 1;
  }

  printf("\n");
  printf("[+] Connected to the NFC emulator device\n");
  printf("[+] Try to break out the auto-emulation, this requires a second reader!\n");
  printf("[+] To do this, please send any command after the anti-collision\n");
  printf("[+] For example, send a RATS command or use the \"nfc-anticol\" tool\n");
  if (!nfc_target_init(pdiTag,abtReaderRx,&szReaderRxBits))
  {
    printf("[+] Initialization of NFC emulator failed\n");
    nfc_disconnect(pdiTag);
    return 1;
  }
  printf("[+] Configuring emulator settings\n");
  nfc_configure(pdiTag,DCO_HANDLE_CRC,false);
  nfc_configure(pdiTag,DCO_HANDLE_PARITY,false);
  nfc_configure(pdiTag,DCO_ACCEPT_INVALID_FRAMES,true);
  printf("[+] Thank you, the emulated tag is initialized\n");

  // Try to open the NFC reader
  pdiReader = INVALID_DEVICE_INFO;
  while (pdiReader == INVALID_DEVICE_INFO) pdiReader = nfc_connect(NULL);
  printf("[+] Configuring NFC reader settings\n");
  nfc_configure(pdiReader,DCO_HANDLE_CRC,false);
  nfc_configure(pdiReader,DCO_HANDLE_PARITY,false);
  nfc_configure(pdiReader,DCO_ACCEPT_INVALID_FRAMES,true);
  printf("[+] Done, relaying frames now!\n\n");

  while(!quitting)
  {
    // Test if we received a frame from the reader
    if (nfc_target_receive_bits(pdiTag,abtReaderRx,&szReaderRxBits,abtReaderRxPar))
    {
      // Drop down the field before sending a REQA command and start a new session
      if (szReaderRxBits == 7 && abtReaderRx[0] == 0x26)
      {
        // Drop down field for a very short time (original tag will reboot)
        nfc_configure(pdiReader,DCO_ACTIVATE_FIELD,false);
        if(!quiet_output)
          printf("\n");
        nfc_configure(pdiReader,DCO_ACTIVATE_FIELD,true);
      }

      // Print the reader frame to the screen
      if(!quiet_output)
      {
        printf("R: ");
        print_hex_par(abtReaderRx,szReaderRxBits,abtReaderRxPar);
      }
      // Forward the frame to the original tag
      if (nfc_initiator_transceive_bits(pdiReader,abtReaderRx,szReaderRxBits,abtReaderRxPar,abtTagRx,&szTagRxBits,abtTagRxPar))
      {
        // Redirect the answer back to the reader
        nfc_target_send_bits(pdiTag,abtTagRx,szTagRxBits,abtTagRxPar);

        // Print the tag frame to the screen
        if(!quiet_output)
        {
          printf("T: ");
          print_hex_par(abtTagRx,szTagRxBits,abtTagRxPar);
        }
      }
    }
  }

  nfc_disconnect(pdiTag);
  nfc_disconnect(pdiReader);
}
