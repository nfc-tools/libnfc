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
 * @file pn532_uart.c
 * @brief
 */
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>

#include "pn532_uart.h"

#include <nfc/nfc-messages.h>

#include "../drivers.h"
#include "../bitutils.h"

// Bus
#include "uart.h"

#ifdef _WIN32
  #define SERIAL_STRING "COM"
  #define delay_ms( X ) Sleep( X )
#else
  // unistd.h is needed for usleep() fct.
  #include <unistd.h>
  #define delay_ms( X ) usleep( X * 1000 )

  #ifdef __APPLE__
    // MacOS
    #define SERIAL_STRING "/dev/tty.SLAB_USBtoUART"
  #else
    // *BSD, Linux and others POSIX systems
    #define SERIAL_STRING "/dev/ttyUSB"
  #endif
#endif

#define BUFFER_LENGTH 256

#define SERIAL_DEFAULT_PORT_SPEED 115200

nfc_device_desc_t *
pn532_uart_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t szN;

    if (!pn532_uart_list_devices (pndd, 1, &szN)) {
      ERR("%s", "pn532_uart_list_devices failed");
      return NULL;
    }

    if (szN == 0) {
      ERR("%s", "No device found");
      return NULL;
    }
  }

  return pndd;
}

bool
pn532_uart_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
/* @note: Due to UART bus we can't know if its really a pn532 without
 * sending some PN53x commands. But using this way to probe devices, we can
 * have serious problem with other device on this bus */
  *pszDeviceFound = 0;

#ifdef DISABLE_SERIAL_AUTOPROBE
  INFO("Sorry, serial auto-probing have been disabled at compile time.");
  return false;
#else /* DISABLE_SERIAL_AUTOPROBE */
  char acConnect[BUFFER_LENGTH];
  serial_port sp;


  // I have no idea how MAC OS X deals with multiple devices, so a quick workaround
  for (int iDevice=0; iDevice<DRIVERS_MAX_DEVICES; iDevice++)
  {
#ifdef __APPLE__
    strcpy(acConnect,SERIAL_STRING);
#else /* __APPLE__ */
    sprintf(acConnect,"%s%d",SERIAL_STRING,iDevice);
#endif /* __APPLE__ */
    sp = uart_open(acConnect);
    DBG("Trying to find PN532 device on serial port: %s at %d bauds.",acConnect, SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT))
    {
      // PN532_UART device found
      uart_close(sp);
      snprintf(pnddDevices[*pszDeviceFound].acDevice, BUFSIZ - 1, "%s (%s)", "PN532", acConnect);
      pnddDevices[*pszDeviceFound].acDevice[BUFSIZ - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = PN532_UART_DRIVER_NAME;
      //pnddDevices[*pszDeviceFound].pcPort = strndup(acConnect, BUFFER_LENGTH - 1);
      pnddDevices[*pszDeviceFound].pcPort = strdup(acConnect);
      pnddDevices[*pszDeviceFound].pcPort[BUFFER_LENGTH] = '\0';
      pnddDevices[*pszDeviceFound].uiSpeed = SERIAL_DEFAULT_PORT_SPEED;
      DBG("Device found: %s.", pnddDevices[*pszDeviceFound].acDevice);
      (*pszDeviceFound)++;

      // Test if we reach the maximum "wanted" devices
      if((*pszDeviceFound) >= szDevices) break;
    }
#ifdef DEBUG
    if (sp == INVALID_SERIAL_PORT) DBG("Invalid serial port: %s",acConnect);
    if (sp == CLAIMED_SERIAL_PORT) DBG("Serial port already claimed: %s",acConnect);
#endif /* DEBUG */
  }
#endif
  return true;
}

nfc_device_t* pn532_uart_connect(const nfc_device_desc_t* pndd)
{
  serial_port sp;
  nfc_device_t* pnd = NULL;

  if( pndd == NULL ) {
    DBG("%s", "pn532_uart_connect() need an nfc_device_desc_t struct.");
  } else {
    DBG("Connecting to: %s at %d bauds.",pndd->pcPort, pndd->uiSpeed);
    sp = uart_open(pndd->pcPort);

    if (sp == INVALID_SERIAL_PORT) ERR("Invalid serial port: %s",pndd->pcPort);
    if (sp == CLAIMED_SERIAL_PORT) ERR("Serial port already claimed: %s",pndd->pcPort);
    if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT)) return NULL;

    uart_set_speed(sp, pndd->uiSpeed);
  }

  /** @info PN532C106 wakeup. */
  /** @todo Put this command in pn53x init process */
  byte_t abtRxBuf[BUFFER_LENGTH];
  size_t szRxBufLen;
  const byte_t pncmd_pn532c106_wakeup[] = { 0x55,0x55,0x00,0x00,0x00,0x00,0x00,0xFF,0x03,0xFD,0xD4,0x14,0x01,0x17,0x00 };

  uart_send(sp, pncmd_pn532c106_wakeup, sizeof(pncmd_pn532c106_wakeup));
  delay_ms(10);

  if (!uart_receive(sp,abtRxBuf,&szRxBufLen)) {
    ERR("%s", "Unable to receive data. (RX)");
    return NULL;
  }
#ifdef DEBUG
  printf(" RX: ");
  print_hex(abtRxBuf,szRxBufLen);
#endif

  DBG("Successfully connected to: %s",pndd->pcPort);

  // We have a connection
  pnd = malloc(sizeof(nfc_device_t));
  strncpy(pnd->acName, pndd->acDevice, DEVICE_NAME_LENGTH - 1);
  pnd->acName[DEVICE_NAME_LENGTH] = '\0';

  pnd->nc = NC_PN532;
  pnd->nds = (nfc_device_spec_t)sp;
  pnd->bActive = true;
  pnd->bCrc = true;
  pnd->bPar = true;
  pnd->ui8TxBits = 0;
  return pnd;
}

void pn532_uart_disconnect(nfc_device_t* pnd)
{
  uart_close((serial_port)pnd->nds);
  free(pnd);
}

bool pn532_uart_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtTxBuf[BUFFER_LENGTH] = { 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"
  byte_t abtRxBuf[BUFFER_LENGTH];
  size_t szRxBufLen = BUFFER_LENGTH;
  size_t szPos;

  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTxBuf[3] = szTxLen;
  // Packet length checksum
  abtTxBuf[4] = BUFFER_LENGTH - abtTxBuf[3];
  // Copy the PN53X command into the packet buffer
  memmove(abtTxBuf+5,pbtTx,szTxLen);

  // Calculate data payload checksum
  abtTxBuf[szTxLen+5] = 0;
  for(szPos=0; szPos < szTxLen; szPos++) 
  {
    abtTxBuf[szTxLen+5] -= abtTxBuf[szPos+5];
  }

  // End of stream marker
  abtTxBuf[szTxLen+6] = 0;

#ifdef DEBUG
  printf(" TX: ");
  print_hex(abtTxBuf,szTxLen+7);
#endif
  if (!uart_send((serial_port)nds,abtTxBuf,szTxLen+7)) {
    ERR("%s", "Unable to transmit data. (TX)");
    return false;
  }

  /** @note PN532 (at 115200 bauds) need 20ms between sending and receiving frame.
   * It seems to be a required delay to able to send from host to device, plus the device computation then device respond transmission 
   */
  delay_ms(20);

  /** @note PN532 (at 115200 bauds) need 30ms more to be stable (report correctly present tag, at each try: 20ms seems to be enought for one shot...)
   * PN532 seems to work correctly with 50ms at 115200 bauds.
   */
  delay_ms(30);

  if (!uart_receive((serial_port)nds,abtRxBuf,&szRxBufLen)) {
    ERR("%s", "Unable to receive data. (RX)");
    return false;
  }

#ifdef DEBUG
  printf(" RX: ");
  print_hex(abtRxBuf,szRxBufLen);
#endif

  // When the answer should be ignored, just return a successful result
  if(pbtRx == NULL || pszRxLen == NULL) return true;

  // Only succeed when the result is at least 00 00 ff 00 ff 00 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if(szRxBufLen < 15) return false;

  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *pszRxLen = szRxBufLen - 15;
  memcpy(pbtRx, abtRxBuf+13, *pszRxLen);

  return true;
}
