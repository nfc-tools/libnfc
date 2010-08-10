/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, 2O1O, Roel Verdult, Romuald Conty
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
 * @file nfc-list.c
 * @brief List the first target present of each founded device
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LIBUSB
  #ifdef DEBUG
    #include <sys/param.h>
    #include <usb.h>
  #endif
#endif

#include <err.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>
#include "nfc-utils.h"

#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 16

static nfc_device_t* pnd;
static byte_t abtFelica[5] = { 0x00, 0xff, 0xff, 0x00, 0x00 };

int main(int argc, const char* argv[])
{
  const char* acLibnfcVersion;
  size_t szDeviceFound;
  size_t szTargetFound;
  size_t i;
  nfc_target_info_t nti;
  nfc_device_desc_t *pnddDevices;
  
  // Display libnfc version
  acLibnfcVersion = nfc_version();
  printf("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  pnddDevices = parse_device_desc(argc, argv, &szDeviceFound);

  if (argc > 1 && szDeviceFound == 0) {
    errx (1, "usage: %s [--device driver:port:speed]", argv[0]);
  }

  #ifdef HAVE_LIBUSB
    #ifdef DEBUG
      usb_set_debug(4);
    #endif
  #endif

  /* Lazy way to open an NFC device */
#if 0
  pnd = nfc_connect(NULL);
#endif

  /* If specific device is wanted, i.e. an ARYGON device on /dev/ttyUSB0 */
#if 0
  nfc_device_desc_t ndd;
  ndd.pcDriver = "ARYGON";
  ndd.pcPort = "/dev/ttyUSB0";
  ndd.uiSpeed = 115200;

  pnd = nfc_connect(&ndd);
#endif

  if (szDeviceFound == 0)
  {
    if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices))))
    {
      fprintf (stderr, "malloc() failed\n");
      return EXIT_FAILURE;
    }

    nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szDeviceFound);
  }

  if (szDeviceFound == 0)
  {
    INFO("%s", "No device found.");
  }

  for (i = 0; i < szDeviceFound; i++)
  {
    nfc_target_info_t anti[MAX_TARGET_COUNT];
    pnd = nfc_connect(&(pnddDevices[i]));


    if (pnd == NULL)
    {
      ERR("%s", "Unable to connect to NFC device.");
      return 1;
    }
    nfc_initiator_init(pnd);

    // Drop the field for a while
    nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);

    // Let the reader only try once to find a tag
    nfc_configure(pnd,NDO_INFINITE_SELECT,false);

    // Configure the CRC and Parity settings
    nfc_configure(pnd,NDO_HANDLE_CRC,true);
    nfc_configure(pnd,NDO_HANDLE_PARITY,true);

    // Enable field so more power consuming cards can power themselves up
    nfc_configure(pnd,NDO_ACTIVATE_FIELD,true);

    printf("Connected to NFC reader: %s\n",pnd->acName);

    if (nfc_initiator_list_passive_targets(pnd, NM_ISO14443A_106, anti, MAX_TARGET_COUNT, &szTargetFound )) {
      size_t n;
      printf("%zu ISO14443A passive targets was found:\n", szTargetFound);
      for(n=0; n<szTargetFound; n++) {
        print_nfc_iso14443a_info (anti[n].nai);
        printf("\n");
      }
    }
    printf("-------------------\n");
    
    // Poll for a Felica tag
    if (nfc_initiator_select_passive_target(pnd,NM_FELICA_212,abtFelica,5,&nti) || nfc_initiator_select_passive_target(pnd,NM_FELICA_424,abtFelica,5,&nti))
    {
      printf("The following (NFC) Felica tag was found:\n\n");
      printf("%18s","ID (NFCID2): "); print_hex(nti.nfi.abtId,8);
      printf("%18s","Parameter (PAD): "); print_hex(nti.nfi.abtPad,8);
    }

    // Poll for a ISO14443B tag
    if (nfc_initiator_select_passive_target(pnd,NM_ISO14443B_106,(byte_t*)"\x00",1,&nti))
    {
      printf("The following (NFC) ISO14443-B tag was found:\n\n");
      printf("  ATQB: "); print_hex(nti.nbi.abtAtqb,12);
      printf("    ID: "); print_hex(nti.nbi.abtId,4);
      printf("   CID: %02x\n",nti.nbi.btCid);
      if (nti.nbi.szInfLen>0)
      {
        printf("   INF: "); print_hex(nti.nbi.abtInf,nti.nbi.szInfLen);
      }
      printf("PARAMS: %02x %02x %02x %02x\n",nti.nbi.btParam1,nti.nbi.btParam2,nti.nbi.btParam3,nti.nbi.btParam4);
    }

    // Poll for a Jewel tag
    if (nfc_initiator_select_passive_target(pnd,NM_JEWEL_106,NULL,0,&nti))
    {
      // No test results yet
      printf("jewel\n");
    }

    nfc_disconnect(pnd);
    }

  free (pnddDevices);
  return 0;
}
