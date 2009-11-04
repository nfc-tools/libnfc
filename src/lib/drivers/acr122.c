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
 * @file acr122.c
 * @brief
 */

#include "acr122.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <winscard.h>

#ifdef __APPLE__
  #include <wintypes.h>
#endif

#include "defines.h"
#include "messages.h"

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

typedef struct {
  SCARDCONTEXT hCtx;
  SCARDHANDLE hCard;
  SCARD_IO_REQUEST ioCard;
} dev_spec_acr122;

dev_info* acr122_connect(const nfc_device_desc_t* pndd)
{
  char* pacReaders[MAX_DEVICES];
  char acList[256+64*MAX_DEVICES];
  size_t szListLen = sizeof(acList);
  size_t szPos;
  uint32_t uiReaderCount;
  uint32_t uiReader;
  uint32_t uiDevIndex;
  dev_info* pdi;
  dev_spec_acr122* pdsa;
  dev_spec_acr122 dsa;
  char* pcFirmware;

  // Clear the reader list
  memset(acList,0x00,szListLen);

  // Test if context succeeded
  if (SCardEstablishContext(SCARD_SCOPE_USER,NULL,NULL,&(dsa.hCtx)) != SCARD_S_SUCCESS) return INVALID_DEVICE_INFO;

  // Retrieve the string array of all available pcsc readers
  if (SCardListReaders(dsa.hCtx,NULL,acList,(void*)&szListLen) != SCARD_S_SUCCESS) return INVALID_DEVICE_INFO;

  DBG("PCSC reports following device(s):");
  DBG("- %s",acList);

  pacReaders[0] = acList;
  uiReaderCount = 1;
  for (szPos=0; szPos<szListLen; szPos++)
  {
    // Make sure don't break out of our reader array
    if (uiReaderCount == MAX_DEVICES) break;

    // Test if there is a next reader available
    if (acList[szPos] == 0x00)
    {
      // Test if we are at the end of the list
      if (acList[szPos+1] == 0x00)
      {
        break;
      }
      // Store the position of the next reader and search for more readers
      pacReaders[uiReaderCount] = acList+szPos+1;
      uiReaderCount++;

      DBG("- %s",acList+szPos+1);
    }
  }

  // Initialize the device index we are seaching for
  if( pndd == NULL ) {
    uiDevIndex = 0;
  } else {
    uiDevIndex = pndd->uiIndex;
  }

  // Iterate through all readers and try to find the ACR122 on requested index
  for (uiReader=0; uiReader<uiReaderCount; uiReader++)
  {
    // Test if we were able to connect to the "emulator" card
    if (SCardConnect(dsa.hCtx,pacReaders[uiReader],SCARD_SHARE_EXCLUSIVE,SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,&(dsa.hCard),(void*)&(dsa.ioCard.dwProtocol)) != SCARD_S_SUCCESS)
    {
      // Connect to ACR122 firmware version >2.0
      if (SCardConnect(dsa.hCtx,pacReaders[uiReader],SCARD_SHARE_DIRECT,0,&(dsa.hCard),(void*)&(dsa.ioCard.dwProtocol)) != SCARD_S_SUCCESS)
      {
        // We can not connect to this device, we will just ignore it
        continue;
      }
    }
    // Configure I/O settings for card communication
    dsa.ioCard.cbPciLength = sizeof(SCARD_IO_REQUEST);

    // Retrieve the current firmware version
    pcFirmware = acr122_firmware((dev_info*)&dsa);
    if (strstr(pcFirmware,FIRMWARE_TEXT) != NULL)
    {
      // We found a occurence, test if it has the right index
      if (uiDevIndex != 0)
      {
        // Let's look for the next reader
        uiDevIndex--;
        continue;
      }

      // Allocate memory and store the device specification
      pdsa = malloc(sizeof(dev_spec_acr122));
      *pdsa = dsa;

      // Done, we found the reader we are looking for
      pdi = malloc(sizeof(dev_info));
      strcpy(pdi->acName,pcFirmware);
      pdi->ct = CT_PN532;
      pdi->ds = (dev_spec)pdsa;
      pdi->bActive = true;
      pdi->bCrc = true;
      pdi->bPar = true;
      pdi->ui8TxBits = 0;
      return pdi;
    }
  }

  // Too bad, the reader could not be located;
  return INVALID_DEVICE_INFO;
}

void acr122_disconnect(dev_info* pdi)
{
  dev_spec_acr122* pdsa = (dev_spec_acr122*)pdi->ds;
  SCardDisconnect(pdsa->hCard,SCARD_LEAVE_CARD);
  SCardReleaseContext(pdsa->hCtx);
  free(pdsa);
  free(pdi);
}

bool acr122_transceive(const dev_spec ds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRxCmd[5] = { 0xFF,0xC0,0x00,0x00 };
  size_t szRxCmdLen = sizeof(abtRxCmd);
  byte_t abtRxBuf[ACR122_RESPONSE_LEN];
  size_t szRxBufLen;
  byte_t abtTxBuf[ACR122_WRAP_LEN+ACR122_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00 };
  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;

  // Make sure the command does not overflow the send buffer
  if (szTxLen > ACR122_COMMAND_LEN) return false;

  // Store the length of the command we are going to send
  abtTxBuf[4] = szTxLen;

  // Prepare and transmit the send buffer
  memcpy(abtTxBuf+5,pbtTx,szTxLen);
  szRxBufLen = sizeof(abtRxBuf);
#ifdef DEBUG
  printf(" TX: ");
  print_hex(abtTxBuf,szTxLen+5);
#endif

  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    if (SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtTxBuf,szTxLen+5,abtRxBuf,szRxBufLen,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  } else {
    if (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtTxBuf,szTxLen+5,NULL,abtRxBuf,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  }

  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_T0)
  {
    // Make sure we received the byte-count we expected
    if (szRxBufLen != 2) return false;

    // Check if the operation was successful, so an answer is available
    if (*abtRxBuf == SCARD_OPERATION_ERROR) return false;

    // Retrieve the response bytes
    abtRxCmd[4] = abtRxBuf[1];
    szRxBufLen = sizeof(abtRxBuf);
    if (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtRxCmd,szRxCmdLen,NULL,abtRxBuf,(void*)&szRxBufLen) != SCARD_S_SUCCESS) return false;
  }

#ifdef DEBUG
  printf(" RX: ");
  print_hex(abtRxBuf,szRxBufLen);
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

char* acr122_firmware(const dev_spec ds)
{
  byte_t abtGetFw[5] = { 0xFF,0x00,0x48,0x00,0x00 };
  uint32_t uiResult;

  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;
  static char abtFw[11];
  size_t szFwLen = sizeof(abtFw);
  memset(abtFw,0x00,szFwLen);
  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    uiResult = SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtGetFw,sizeof(abtGetFw),abtFw,szFwLen,(void*)&szFwLen);
  } else {
    uiResult = SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtGetFw,sizeof(abtGetFw),NULL,(byte_t*)abtFw,(void*)&szFwLen);
  }

  #ifdef DEBUG
  if (uiResult != SCARD_S_SUCCESS)
  {
    printf("No ACR122 firmware received, Error: %08x\n",uiResult);
  }
  #endif

  return abtFw;
}

bool acr122_led_red(const dev_spec ds, bool bOn)
{
  byte_t abtLed[9] = { 0xFF,0x00,0x40,0x05,0x04,0x00,0x00,0x00,0x00 };
  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;
  byte_t abtBuf[2];
  size_t szBufLen = sizeof(abtBuf);
  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    return (SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtLed,sizeof(abtLed),abtBuf,szBufLen,(void*)&szBufLen) == SCARD_S_SUCCESS);
  } else {
    return (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtLed,sizeof(abtLed),NULL,(byte_t*)abtBuf,(void*)&szBufLen) == SCARD_S_SUCCESS);
  }
}

