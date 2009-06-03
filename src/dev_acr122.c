/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "defines.h"

#include <winscard.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dev_acr122.h"
#include "bitutils.h"

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

static byte abtTxBuf[ACR122_WRAP_LEN+ACR122_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00 };
static byte abtRxCmd[5] = { 0xFF,0xC0,0x00,0x00 };
static byte uiRxCmdLen = sizeof(abtRxCmd);
static byte abtRxBuf[ACR122_RESPONSE_LEN];
static ulong ulRxBufLen;
static byte abtGetFw[5] = { 0xFF,0x00,0x48,0x00,0x00 };
static byte abtLed[9] = { 0xFF,0x00,0x40,0x05,0x04,0x00,0x00,0x00,0x00 };

dev_info* dev_acr122_connect(const ui32 uiIndex)
{
  char* pacReaders[MAX_DEVICES];
  char acList[256+64*MAX_DEVICES];
  ulong ulListLen = sizeof(acList);
  ui32 uiPos;
  ui32 uiReaderCount;
  ui32 uiReader;
  ui32 uiDevIndex;
  dev_info* pdi;
  dev_spec_acr122* pdsa;
  dev_spec_acr122 dsa;
  char* pcFirmware;

  // Clear the reader list
  memset(acList,0x00,ulListLen);

  // Test if context succeeded
  if (SCardEstablishContext(SCARD_SCOPE_USER,null,null,&(dsa.hCtx)) != SCARD_S_SUCCESS) return INVALID_DEVICE_INFO;

  // Retrieve the string array of all available pcsc readers
  if (SCardListReaders(dsa.hCtx,null,acList,(void*)&ulListLen) != SCARD_S_SUCCESS) return INVALID_DEVICE_INFO;
  
  #ifdef _LIBNFC_VERBOSE_
      printf("Found the following PCSC device(s)\n");
      printf("- %s\n",acList);
  #endif

  pacReaders[0] = acList;
  uiReaderCount = 1;
  for (uiPos=0; uiPos<ulListLen; uiPos++)
  {
    // Make sure don't break out of our reader array
    if (uiReaderCount == MAX_DEVICES) break;

    // Test if there is a next reader available
    if (acList[uiPos] == 0x00)
    {
      // Test if we are at the end of the list
      if (acList[uiPos+1] == 0x00)
      {
        break;
      }
      // Store the position of the next reader and search for more readers
      pacReaders[uiReaderCount] = acList+uiPos+1;
      uiReaderCount++;

      // Debug info
      #ifdef _LIBNFC_VERBOSE_
        printf("- %s\n",acList+uiPos+1);
      #endif
    }
  }

  // Initialize the device index we are seaching for
  uiDevIndex = uiIndex;
  
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
    pcFirmware = dev_acr122_firmware((dev_info*)&dsa);
    if (strstr(pcFirmware,FIRMWARE_TEXT) != null)
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

void dev_acr122_disconnect(dev_info* pdi)
{
  dev_spec_acr122* pdsa = (dev_spec_acr122*)pdi->ds;
  SCardDisconnect(pdsa->hCard,SCARD_LEAVE_CARD);
  SCardReleaseContext(pdsa->hCtx);
  free(pdsa);
  free(pdi);
}

bool dev_acr122_transceive(const dev_spec ds, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen)
{
  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;

  // Make sure the command does not overflow the send buffer
  if (uiTxLen > ACR122_COMMAND_LEN) return false;
    
  // Store the length of the command we are going to send
  abtTxBuf[4] = uiTxLen;

  // Prepare and transmit the send buffer
  memcpy(abtTxBuf+5,pbtTx,uiTxLen);
  ulRxBufLen = sizeof(abtRxBuf);
  #ifdef _LIBNFC_VERBOSE_
    printf("Tx: ");
    print_hex(abtTxBuf,uiTxLen+5);
  #endif

  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    if (SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtTxBuf,uiTxLen+5,abtRxBuf,ulRxBufLen,(void*)&ulRxBufLen) != SCARD_S_SUCCESS) return false;
  } else {
    if (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtTxBuf,uiTxLen+5,null,abtRxBuf,(void*)&ulRxBufLen) != SCARD_S_SUCCESS) return false;
  }

  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_T0)
  {
    // Make sure we received the byte-count we expected
    if (ulRxBufLen != 2) return false;

    // Check if the operation was successful, so an answer is available
    if (*abtRxBuf == SCARD_OPERATION_ERROR) return false;

    // Retrieve the response bytes
    abtRxCmd[4] = abtRxBuf[1];
    ulRxBufLen = sizeof(abtRxBuf);
    if (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtRxCmd,uiRxCmdLen,null,abtRxBuf,(void*)&ulRxBufLen) != SCARD_S_SUCCESS) return false;
  }

  #ifdef _LIBNFC_VERBOSE_
    printf("Rx: ");
    print_hex(abtRxBuf,ulRxBufLen);
  #endif

  // When the answer should be ignored, just return a succesful result
  if (pbtRx == null || puiRxLen == null) return true;

  // Make sure we have an emulated answer that fits the return buffer
  if (ulRxBufLen < 4 || (ulRxBufLen-4) > *puiRxLen) return false;
  // Wipe out the 4 APDU emulation bytes: D5 4B .. .. .. 90 00
  *puiRxLen = ulRxBufLen-4;
  memcpy(pbtRx,abtRxBuf+2,*puiRxLen);

  // Transmission went successful
  return true;
}

char* dev_acr122_firmware(const dev_spec ds)
{
  ui32 uiResult;
  
  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;
  static char abtFw[11];
  ulong ulFwLen = sizeof(abtFw);
  memset(abtFw,0x00,ulFwLen);
  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    uiResult = SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtGetFw,sizeof(abtGetFw),abtFw,ulFwLen,(void*)&ulFwLen);
  } else {
    uiResult = SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtGetFw,sizeof(abtGetFw),null,(byte*)abtFw,(void*)&ulFwLen);
  }

  #ifdef _LIBNFC_VERBOSE_
  if (uiResult != SCARD_S_SUCCESS)
  {
    printf("No ACR122 firmware received, Error: %08x\n",uiResult);
  }
  #endif

  return abtFw;
}

bool dev_acr122_led_red(const dev_spec ds, bool bOn)
{
  dev_spec_acr122* pdsa = (dev_spec_acr122*)ds;
  byte abtBuf[2];
  ulong ulBufLen = sizeof(abtBuf);
  if (pdsa->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED)
  {
    return (SCardControl(pdsa->hCard,IOCTL_CCID_ESCAPE_SCARD_CTL_CODE,abtLed,sizeof(abtLed),abtBuf,ulBufLen,(void*)&ulBufLen) == SCARD_S_SUCCESS);
  } else {
    return (SCardTransmit(pdsa->hCard,&(pdsa->ioCard),abtLed,sizeof(abtLed),null,(byte*)abtBuf,(void*)&ulBufLen) == SCARD_S_SUCCESS);
  }
}

