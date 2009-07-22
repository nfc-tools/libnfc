/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#include "dev_arygon.h"
#include "rs232.h"
#include "bitutils.h"

#ifdef _WIN32
  #define SERIAL_STRING "COM"
#else
  #ifdef __APPLE__
    #define SERIAL_STRING "/dev/tty.SLAB_USBtoUART"
  #else
    #define SERIAL_STRING "/dev/ttyUSB"
  #endif
#endif

#define BUFFER_LENGTH 256
#define USB_TIMEOUT   30000
static byte_t abtTxBuf[BUFFER_LENGTH] = { 0x32, 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"

dev_info* dev_arygon_connect(const uint32_t uiIndex)
{
  uint32_t uiDevNr;
  serial_port sp;
  char acConnect[BUFFER_LENGTH];
  dev_info* pdi = INVALID_DEVICE_INFO;
  
#ifdef DEBUG
  printf("Trying to find ARYGON device on serial port: %s#\n",SERIAL_STRING);
#endif
  
  // I have no idea how MAC OS X deals with multiple devices, so a quick workaround
  for (uiDevNr=0; uiDevNr<MAX_DEVICES; uiDevNr++)
  {
    #ifdef __APPLE__
      strcpy(acConnect,SERIAL_STRING);
      sp = rs232_open(acConnect);
    #else
      sprintf(acConnect,"%s%d",SERIAL_STRING,uiDevNr);
      sp = rs232_open(acConnect);
    #endif
    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) break;
    #ifdef DEBUG
      if (sp == INVALID_SERIAL_PORT) printf("invalid serial port: %s\n",acConnect);
      if (sp == CLAIMED_SERIAL_PORT) printf("serial port already claimed: %s\n",acConnect);
    #endif
  }
  // Test if we have found a device
  if (uiDevNr == MAX_DEVICES) return INVALID_DEVICE_INFO;
  
#ifdef DEBUG
  printf("Succesfully connected to: %s\n",acConnect);
#endif
  
  // We have a connection
  pdi = malloc(sizeof(dev_info));
  strcpy(pdi->acName,"ARYGON");
  pdi->ct = CT_PN532;
  pdi->ds = (dev_spec)sp;
  pdi->bActive = true;
  pdi->bCrc = true;
  pdi->bPar = true;
  pdi->ui8TxBits = 0;
  return pdi;
}                                                                                          

void dev_arygon_disconnect(dev_info* pdi)
{
  rs232_close((serial_port)pdi->ds);
  free(pdi);
}                                        

bool dev_arygon_transceive(const dev_spec ds, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen)
{
  byte_t abtRxBuf[BUFFER_LENGTH];
  uint32_t uiRxBufLen = BUFFER_LENGTH;
  uint32_t uiPos;
    
  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTxBuf[4] = uiTxLen;                                                                
  // Packet length checksum 
  abtTxBuf[5] = BUFFER_LENGTH - abtTxBuf[4];                                                  
  // Copy the PN53X command into the packet buffer
  memmove(abtTxBuf+6,pbtTx,uiTxLen);
  
  // Calculate data payload checksum
  abtTxBuf[uiTxLen+6] = 0;                   
  for(uiPos=0; uiPos < uiTxLen; uiPos++) 
  {
    abtTxBuf[uiTxLen+6] -= abtTxBuf[uiPos+6];
  }
  
  // End of stream marker
  abtTxBuf[uiTxLen+7] = 0;        
  
#ifdef DEBUG
  printf("Tx: ");
  print_hex(abtTxBuf,uiTxLen+8);
#endif
  if (!rs232_send((serial_port)ds,abtTxBuf,uiTxLen+8)) return false;

  if (!rs232_receive((serial_port)ds,abtRxBuf,&uiRxBufLen)) return false;
  

#ifdef DEBUG
  printf("Rx: ");
  print_hex(abtRxBuf,uiRxBufLen);
#endif
  
  // When the answer should be ignored, just return a succesful result    
  if(pbtRx == NULL || puiRxLen == NULL) return true;

  // Only succeed when the result is at least 00 00 ff 00 ff 00 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if(uiRxBufLen < 15) return false;
  
  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *puiRxLen = uiRxBufLen - 15;
  memcpy(pbtRx, abtRxBuf+13, *puiRxLen);
  
  return true;
}
