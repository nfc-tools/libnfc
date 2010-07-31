/*-
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
 */

/**
 * @file acr122.c
 * @brief Driver for ACR122 devices (e.g. Tikitag, Touchatag, ACS ACR122)
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "acr122.h"
#include "../drivers.h"

// Bus
#include <winscard.h>

#ifdef __APPLE__
  #include <wintypes.h>
#endif


#include <nfc/nfc-messages.h>

// WINDOWS: #define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(3500)
#define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE (((0x31) << 16) | ((3500) << 2))
#define SCARD_OPERATION_SUCCESS	0x61
#define SCARD_OPERATION_ERROR	0x63

#ifndef SCARD_PROTOCOL_UNDEFINED
  #define SCARD_PROTOCOL_UNDEFINED SCARD_PROTOCOL_UNSET
#endif

#define FIRMWARE_TEXT "ACR122U" // Tested on: ACR122U101(ACS), ACR122U102(Tikitag), ACR122U203(ACS)

#define ACR122_WRAP_LEN 5
#define ACR122_COMMAND_LEN 266
#define ACR122_RESPONSE_LEN 268

const char *supported_devices[] = {
  "ACS ACR122",
  "ACS ACR 38U-CCID",
  "    CCID USB",
  NULL
};

typedef struct {
  SCARDHANDLE hCard;
  SCARD_IO_REQUEST ioCard;
} acr122_spec_t;

static SCARDCONTEXT _SCardContext;
static int _iSCardContextRefCount = 0;

SCARDCONTEXT*
acr122_get_scardcontext(void)
{
  if ( _iSCardContextRefCount == 0 )
  {
    if (SCardEstablishContext(SCARD_SCOPE_USER,NULL,NULL,&_SCardContext) != SCARD_S_SUCCESS) return NULL;
  }
  _iSCardContextRefCount++;

  return &_SCardContext;
}

void
acr122_free_scardcontext(void)
{
  if (_iSCardContextRefCount)
  {
    _iSCardContextRefCount--;
    if (!_iSCardContextRefCount)
    {
      SCardReleaseContext(_SCardContext);
    }
  }
}


nfc_device_desc_t *
acr122_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t szN;

    if (!acr122_list_devices (pndd, 1, &szN)) {
      DBG("%s", "acr122_list_devices failed");
      return NULL;
    }

    if (szN == 0) {
      DBG("%s", "No device found");
      return NULL;
    }
  }

  return pndd;
}

/**
 * @brief List connected devices
 *
 * Probe PCSC to find NFC capable hardware.
 *
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 * @return true if succeeded, false otherwise.
 */
bool
acr122_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
  size_t szPos = 0;
  char acDeviceNames[256+64*DRIVERS_MAX_DEVICES];
  size_t szDeviceNamesLen = sizeof(acDeviceNames);
  uint32_t uiBusIndex = 0;
  SCARDCONTEXT *pscc;
  bool bSupported;
  int i;

  // Clear the reader list
  memset(acDeviceNames, '\0', szDeviceNamesLen);

  *pszDeviceFound = 0;

  // Test if context succeeded
  if (!(pscc = acr122_get_scardcontext ()))
  {
    DBG("%s","PCSC context not found");
    return false;
  }

  // Retrieve the string array of all available pcsc readers
  if (SCardListReaders(*pscc,NULL,acDeviceNames,(void*)&szDeviceNamesLen) != SCARD_S_SUCCESS) return false;

  // DBG("%s", "PCSC reports following device(s):");

  while ((acDeviceNames[szPos] != '\0') && ((*pszDeviceFound) < szDevices)) {
    uiBusIndex++;

    // DBG("- %s (pos=%ld)", acDeviceNames + szPos, (unsigned long) szPos);

    bSupported = false;
    for (i = 0; supported_devices[i] && !bSupported; i++) {
      int l = strlen(supported_devices[i]);
      bSupported = 0 == strncmp(supported_devices[i], acDeviceNames + szPos, l);
    }

    if (bSupported)
    {
      // Supported ACR122 device found
      strncpy(pnddDevices[*pszDeviceFound].acDevice, acDeviceNames + szPos, DEVICE_NAME_LENGTH - 1);
      pnddDevices[*pszDeviceFound].acDevice[DEVICE_NAME_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = ACR122_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].uiBusIndex = uiBusIndex;
      (*pszDeviceFound)++;
    }
    else
    {
      DBG("PCSC device [%s] is not NFC capable or not supported by libnfc.", acDeviceNames + szPos);
    }

    // Find next device name position
    while (acDeviceNames[szPos++] != '\0');
  }
  acr122_free_scardcontext ();

  if(*pszDeviceFound)
    return true;
  return false;
}

nfc_device_t* acr122_connect(const nfc_device_desc_t* pndd)
{
  nfc_device_t* pnd = NULL;
  acr122_spec_t as;
  acr122_spec_t* pas;
  char* pcFirmware;

  SCARDCONTEXT *pscc;

  DBG("Attempt to connect to %s",pndd->acDevice);
  // We no longer support connecting with a NULL
  if(pndd == NULL) return NULL;
  // Test if context succeeded
  if (!(pscc = acr122_get_scardcontext ())) return NULL;
  // Test if we were able to connect to the "emulator" card
  if (SCardConnect(*pscc,pndd->acDevice,SCARD_SHARE_EXCLUSIVE,SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,&(as.hCard),(void*)&(as.ioCard.dwProtocol)) != SCARD_S_SUCCESS)
  {
    // Connect to ACR122 firmware version >2.0
    if (SCardConnect(*pscc,pndd->acDevice,SCARD_SHARE_DIRECT,0,&(as.hCard),(void*)&(as.ioCard.dwProtocol)) != SCARD_S_SUCCESS)
    {
      // We can not connect to this device.
      DBG("%s","PCSC connect failed");
      return NULL;
    }
  }
  // Configure I/O settings for card communication
  as.ioCard.cbPciLength = sizeof(SCARD_IO_REQUEST);

  // Retrieve the current firmware version
  pcFirmware = acr122_firmware((nfc_device_t*)&as);
  if (strstr(pcFirmware,FIRMWARE_TEXT) != NULL)
  {
    // Allocate memory and store the device specification
    pas = malloc(sizeof(acr122_spec_t));
    *pas = as;

    // Done, we found the reader we are looking for
    pnd = malloc(sizeof(nfc_device_t));
    strcpy(pnd->acName,pndd->acDevice);
    strcpy(pnd->acName + strlen(pnd->acName)," / ");
    strcpy(pnd->acName + strlen(pnd->acName),pcFirmware);
    pnd->nc = NC_PN532;
    pnd->nds = (nfc_device_spec_t)pas;
    pnd->bActive = true;
    pnd->bCrc = true;
    pnd->bPar = true;
    pnd->ui8TxBits = 0;
    return pnd;
  }

  return NULL;
}

void acr122_disconnect(nfc_device_t* pnd)
{
  acr122_spec_t* pas = (acr122_spec_t*)pnd->nds;
  SCardDisconnect(pas->hCard,SCARD_LEAVE_CARD);
  acr122_free_scardcontext ();
  free(pas);
  free(pnd);
}

bool acr122_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRxCmd[5] = { 0xFF,0xC0,0x00,0x00 };
  size_t szRxCmdLen = sizeof(abtRxCmd);
  byte_t abtRxBuf[ACR122_RESPONSE_LEN];
  size_t szRxBufLen;
  byte_t abtTxBuf[ACR122_WRAP_LEN+ACR122_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00 };
  acr122_spec_t* pas = (acr122_spec_t*)nds;

  // Make sure the command does not overflow the send buffer
  if (szTxLen > ACR122_COMMAND_LEN) return false;

  // Store the length of the command we are going to send
  abtTxBuf[4] = szTxLen;

  // Prepare and transmit the send buffer
  memcpy(abtTxBuf+5,pbtTx,szTxLen);
  szRxBufLen = sizeof(abtRxBuf);
#ifdef DEBUG
  PRINT_HEX("TX", abtTxBuf,szTxLen+5);
#endif

  if (pas->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    if (SCardControl(pas->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtTxBuf,szTxLen+5,abtRxBuf,szRxBufLen,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  } else {
    if (SCardTransmit(pas->hCard,&(pas->ioCard),abtTxBuf,szTxLen+5,NULL,abtRxBuf,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  }

  if (pas->ioCard.dwProtocol == SCARD_PROTOCOL_T0)
  {
    // Make sure we received the byte-count we expected
    if (szRxBufLen != 2) return false;

    // Check if the operation was successful, so an answer is available
    if (*abtRxBuf == SCARD_OPERATION_ERROR) return false;

    // Retrieve the response bytes
    abtRxCmd[4] = abtRxBuf[1];
    szRxBufLen = sizeof(abtRxBuf);
    if (SCardTransmit(pas->hCard,&(pas->ioCard),abtRxCmd,szRxCmdLen,NULL,abtRxBuf,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  }

#ifdef DEBUG
  PRINT_HEX("RX", abtRxBuf,szRxBufLen);
#endif

  // When the answer should be ignored, just return a succesful result
  if (pbtRx == NULL || pszRxLen == NULL) return true;

  // Make sure we have an emulated answer that fits the return buffer
  if (szRxBufLen < 4 || (szRxBufLen-4) > *pszRxLen) return false;
  // Wipe out the 4 APDU emulation bytes: D5 4B .. .. .. 90 00
  *pszRxLen = ((size_t)szRxBufLen)-4;
  memcpy(pbtRx,abtRxBuf+2,*pszRxLen);

  // Transmission went successful
  return true;
}

char* acr122_firmware(const nfc_device_spec_t nds)
{
  byte_t abtGetFw[5] = { 0xFF,0x00,0x48,0x00,0x00 };
  uint32_t uiResult;

  acr122_spec_t* pas = (acr122_spec_t*)nds;
  static char abtFw[11];
  size_t szFwLen = sizeof(abtFw);
  memset(abtFw,0x00,szFwLen);
  if (pas->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    uiResult = SCardControl(pas->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtGetFw,sizeof(abtGetFw),abtFw,szFwLen,(void*)&szFwLen);
  } else {
    uiResult = SCardTransmit(pas->hCard,&(pas->ioCard),abtGetFw,sizeof(abtGetFw),NULL,(byte_t*)abtFw,(void*)&szFwLen);
  }

  #ifdef DEBUG
  if (uiResult != SCARD_S_SUCCESS)
  {
    printf("No ACR122 firmware received, Error: %08x\n",uiResult);
  }
  #endif

  return abtFw;
}

bool acr122_led_red(const nfc_device_spec_t nds, bool bOn)
{
  byte_t abtLed[9] = { 0xFF,0x00,0x40,0x05,0x04,0x00,0x00,0x00,0x00 };
  acr122_spec_t* pas = (acr122_spec_t*)nds;
  byte_t abtBuf[2];
  size_t szBufLen = sizeof(abtBuf);
  (void)bOn;
  if (pas->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    return (SCardControl(pas->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtLed,sizeof(abtLed),abtBuf,szBufLen,(void*)&szBufLen) == SCARD_S_SUCCESS);
  } else {
    return (SCardTransmit(pas->hCard,&(pas->ioCard),abtLed,sizeof(abtLed),NULL,(byte_t*)abtBuf,(void*)&szBufLen) == SCARD_S_SUCCESS);
  }
}

