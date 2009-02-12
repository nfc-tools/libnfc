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

#include <winscard.h>
#include <stdio.h>
#include <string.h>
#include "acr122.h"
#include "bitutils.h"

#define SCARD_OPERATION_SUCCESS	0x61
#define SCARD_OPERATION_ERROR	0x63

#define FIRMWARE_TEXT "ACR122U10" // ACR122U101(ACS), ACR122U102(Tikitag)

#define ACR122_WRAP_LEN 5
#define ACR122_COMMAND_LEN 266

#define ACR122_RESPONSE_LEN 268

#define MAX_READERS 16

static SCARDCONTEXT hCtx = null;

static byte abtTxBuf[ACR122_WRAP_LEN+ACR122_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00 };
static byte abtRxCmd[5] = { 0xFF,0xC0,0x00,0x00 };
static byte uiRxCmdLen = sizeof(abtRxCmd);
static byte abtRxBuf[ACR122_RESPONSE_LEN];
static ulong ulRxBufLen;
static byte abtGetFw[5] = { 0xFF,0x00,0x48,0x00,0x00 };
static byte abtLed[9] = { 0xFF,0x00,0x40,0x05,0x04,0x00,0x00,0x00,0x00 };

dev_id acr122_connect(const ui32 uiDeviceIndex)
{
  SCARDHANDLE hCard = null;
  char* pacReaders[MAX_READERS];
  char acList[256+64*MAX_READERS];
  ulong ulListLen = sizeof(acList);
  ulong ulActiveProtocol = 0;
  ui32 uiPos;
  ui32 uiReaderCount;
  ui32 uiReader;
  ui32 uiReaderIndex;

  // Clear the reader list
  memset(acList,0x00,ulListLen);

  // Test if context succeeded
  if (SCardEstablishContext(SCARD_SCOPE_USER,null,null,&hCtx) != SCARD_S_SUCCESS) return INVALID_DEVICE_ID;

  // Retrieve the string array of all available pcsc readers
  if (SCardListReaders(hCtx,null,acList,(void*)&ulListLen) != SCARD_S_SUCCESS) return INVALID_DEVICE_ID;
  
  pacReaders[0] = acList;
  uiReaderCount = 1;
  for (uiPos=0; uiPos<ulListLen; uiPos++)
  {
    // Make sure don't break out of our reader array
    if (uiReaderCount == MAX_READERS) break;

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
    }
  }

  // Initialize the reader index we are seaching for
  uiReaderIndex = uiDeviceIndex;
  
  // Iterate through all readers and try to find the ACR122 on requested index
  for (uiReader=0; uiReader<uiReaderCount; uiReader++)
  {
    // Test if we were able to connect to the "emulator" card
    if (SCardConnect(hCtx,pacReaders[uiReader],SCARD_SHARE_SHARED,SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,&hCard,(void*)&ulActiveProtocol) == SCARD_S_SUCCESS)
    {
      if (strstr(acr122_firmware((void*)hCard),FIRMWARE_TEXT) != null)
      {
        // We found a occurence, test if it has the right index
        if (uiReaderIndex == 0)
        {
          // Done, we found the reader we are looking for
          return (void*)hCard;
        } else {
          // Let's look for the next reader
          uiReaderIndex--;
        }
      }
    }
  }

  // Too bad, the reader could not be located;
  return INVALID_DEVICE_ID;
}

void acr122_disconnect(const dev_id di)
{
  SCardDisconnect((SCARDHANDLE)di,SCARD_LEAVE_CARD);
}

bool acr122_transceive(const dev_id di, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen)
{
  if (di == null) return false;

  // Make sure the command does not overflow the send buffer
  if (uiTxLen > ACR122_COMMAND_LEN) return false;
    
  // Store the length of the command we are going to send
  abtTxBuf[4] = uiTxLen;

  #ifdef _LIBNFC_VERBOSE_
    printf("Tx: ");
    print_hex(pbtTx,uiTxLen);
  #endif
  // Prepare and transmit the send buffer
  memcpy(abtTxBuf+5,pbtTx,uiTxLen);
  ulRxBufLen = sizeof(abtRxBuf);
  if (SCardTransmit((SCARDHANDLE)di,SCARD_PCI_T0,abtTxBuf,uiTxLen+5,null,abtRxBuf,(void*)&ulRxBufLen) != SCARD_S_SUCCESS) return false;

  // Make sure we received the byte-count we expected
  if (ulRxBufLen != 2) return false;

  // Check if the operation was successful, so an answer is available
  if (*abtRxBuf == SCARD_OPERATION_ERROR) return false;

  // Retrieve the response bytes
  abtRxCmd[4] = abtRxBuf[1];
  ulRxBufLen = sizeof(abtRxBuf);
  if (SCardTransmit((SCARDHANDLE)di,SCARD_PCI_T0,abtRxCmd,uiRxCmdLen,null,abtRxBuf,(void*)&ulRxBufLen) != SCARD_S_SUCCESS) return false;

  // When the answer should be ignored, just return true
  if (pbtRx == null || puiRxLen == null) return true;

  // Make sure we have an emulated answer that fits the return buffer
  if (ulRxBufLen < 4 || (ulRxBufLen-4) > *puiRxLen) return false;

  // Wipe out the 4 APDU emulation bytes: D5 4B .. .. .. 90 00
  *puiRxLen = ulRxBufLen-4;
  memcpy(pbtRx,abtRxBuf+2,*puiRxLen);
  #ifdef _LIBNFC_VERBOSE_
    printf("Rx: ");
    print_hex(pbtRx,*puiRxLen);
  #endif

  // Transmission went successful
  return true;
}

char* acr122_firmware(const dev_id di)
{
  static char abtFw[11];
  ulong ulFwLen = sizeof(abtFw);
  memset(abtFw,0x00,ulFwLen);
  SCardTransmit((SCARDHANDLE)di,SCARD_PCI_T0,abtGetFw,sizeof(abtGetFw),null,(byte*)abtFw,(void*)&ulFwLen);
  return abtFw;
}

bool acr122_led_red(const dev_id di, bool bOn)
{
  byte abtBuf[2];
  ulong ulBufLen = sizeof(abtBuf);
  return (SCardTransmit((SCARDHANDLE)di,SCARD_PCI_T0,abtLed,sizeof(abtLed),null,(byte*)abtBuf,(void*)&ulBufLen) == SCARD_S_SUCCESS);
}

