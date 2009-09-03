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
    // unistd.h is needed for udelay() fct.
    #include "unistd.h"
    #define SERIAL_STRING "/dev/ttyUSB"
  #endif
#endif

#define BUFFER_LENGTH 256
#define USB_TIMEOUT   30000

/** @def DEV_ARYGON_PROTOCOL_ARYGON_ASCII
 * @brief High level language in ASCII format. (Common µC commands and Mifare® commands) 
 */
#define DEV_ARYGON_PROTOCOL_ARYGON_ASCII        '0'
/** @def DEV_ARYGON_MODE_HL_ASCII
 * @brief High level language in Binary format With AddressingByte for party line. (Common µC commands and Mifare® commands) 
 */
#define DEV_ARYGON_PROTOCOL_ARYGON_BINARY_WAB   '1'
/** @def DEV_ARYGON_PROTOCOL_TAMA
 * @brief Philips protocol (TAMA language) in binary format.
 */
#define DEV_ARYGON_PROTOCOL_TAMA                '2'
/** @def DEV_ARYGON_PROTOCOL_TAMA_WAB
 * @brief Philips protocol (TAMA language) in binary With AddressingByte for party line.
 */
#define DEV_ARYGON_PROTOCOL_TAMA_WAB            '3'

static byte_t abtTxBuf[BUFFER_LENGTH] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"

dev_info* dev_arygon_connect(const uint32_t uiIndex)
{
  uint32_t uiDevNr;
  serial_port sp;
  char acConnect[BUFFER_LENGTH];
  dev_info* pdi = INVALID_DEVICE_INFO;

  DBG("Trying to find ARYGON device on serial port: %s#",SERIAL_STRING);

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
      if (sp == INVALID_SERIAL_PORT) DBG("Invalid serial port: %s",acConnect);
      if (sp == CLAIMED_SERIAL_PORT) DBG("Serial port already claimed: %s",acConnect);
    #endif
  }
  // Test if we have found a device
  if (uiDevNr == MAX_DEVICES) return INVALID_DEVICE_INFO;

  DBG("Successfully connected to: %s",acConnect);

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
  printf(" TX: ");
  print_hex(abtTxBuf,uiTxLen+8);
#endif
  if (!rs232_send((serial_port)ds,abtTxBuf,uiTxLen+8)) {
    ERR("Unable to transmit data. (TX)");
    return false;
  }

  /** @note ARYGON-APDB need 20ms between sending and receiving frame. No information regarding this in ARYGON datasheet... */
  usleep(20000);

  /** @note ARYGON-APDB need 20ms more to be able to report (correctly) present tag. */
  usleep(20000);

  if (!rs232_receive((serial_port)ds,abtRxBuf,&uiRxBufLen)) {
    ERR("Unable to receive data. (RX)");
    return false;
  }

#ifdef DEBUG
  printf(" RX: ");
  print_hex(abtRxBuf,uiRxBufLen);
#endif

  // When the answer should be ignored, just return a successful result
  if(pbtRx == NULL || puiRxLen == NULL) return true;

  // Only succeed when the result is at least 00 00 ff 00 ff 00 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if(uiRxBufLen < 15) return false;

  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *puiRxLen = uiRxBufLen - 15;
  memcpy(pbtRx, abtRxBuf+13, *puiRxLen);

  return true;
}
