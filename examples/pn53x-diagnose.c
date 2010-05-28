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
 * @file pn53x-diagnose.c
 * @brief Small application to diagnose PN53x using dedicated commands
 */
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>

#include "nfc-utils.h"
#include "chips/pn53x.h"

#define MAX_DEVICE_COUNT 16


int main(int argc, const char* argv[])
{
  size_t szFound;
  size_t i;
  nfc_device_t* pnd;
  nfc_device_desc_t *pnddDevices;
  const char* acLibnfcVersion;
  bool result;

  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  const byte_t pncmd_diagnose_communication_line_test[] = { 0xD4, 0x00, 0x00, 0x06, 'l', 'i', 'b', 'n', 'f', 'c' };
  const byte_t pncmd_diagnose_rom_test[] = { 0xD4, 0x00, 0x01 };
  const byte_t pncmd_diagnose_ram_test[] = { 0xD4, 0x00, 0x02 };

  if (argc > 1) {
    errx (1, "usage: %s", argv[0]);
  }

  // Display libnfc version
  acLibnfcVersion = nfc_version();
  printf("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  if (!(pnddDevices = malloc (MAX_DEVICE_COUNT * sizeof (*pnddDevices)))) {
    fprintf (stderr, "malloc() failed\n");
    return EXIT_FAILURE;
  }

  nfc_list_devices (pnddDevices, MAX_DEVICE_COUNT, &szFound);

  if (szFound == 0) {
    INFO("%s", "No device found.");
  }

  for (i = 0; i < szFound; i++) {
    pnd = nfc_connect(&(pnddDevices[i]));

    if (pnd == NULL) {
      ERR("%s", "Unable to connect to NFC device.");
      return EXIT_FAILURE;
    }

    printf("NFC device [%s] connected.\n",pnd->acName);

    result = pnd->pdc->transceive(pnd->nds,pncmd_diagnose_communication_line_test,sizeof(pncmd_diagnose_communication_line_test),abtRx,&szRxLen);
    if ( result ) {
      result = (memcmp(pncmd_diagnose_communication_line_test+2, abtRx, sizeof(pncmd_diagnose_communication_line_test)-2 ) == 0);
    }
    printf(" Communication line test: %s\n", result ? "OK" : "Failed");

    result = pnd->pdc->transceive(pnd->nds,pncmd_diagnose_rom_test,sizeof(pncmd_diagnose_rom_test),abtRx,&szRxLen);
    if ( result ) {
      result = ((szRxLen == 1) && (abtRx[0] == 0x00));
    }
    printf(" ROM test: %s\n", result ? "OK" : "Failed");

    result = pnd->pdc->transceive(pnd->nds,pncmd_diagnose_ram_test,sizeof(pncmd_diagnose_ram_test),abtRx,&szRxLen);
    if ( result ) {
      result = ((szRxLen == 1) && (abtRx[0] == 0x00));
    }
    printf(" RAM test: %s\n", result ? "OK" : "Failed");
  }
}