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
 * @file pn532_uart.c
 * @brief PN532 driver using UART bus (UART, RS232, etc.)
 */

#ifdef DRIVER_PN532_UART_ENABLED

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include "../drivers.h"

#include <stdio.h>
#include <string.h>

#include "pn532_uart.h"

#include <nfc/nfc-messages.h>

// Bus
#include "uart.h"

#ifdef _WIN32
  #define SERIAL_STRING "COM"
  #define snprintf _snprintf
  #define strdup _strdup
  #define delay_ms( X ) Sleep( X )
#else
  // unistd.h is needed for usleep() fct.
  #include <unistd.h>
  #define delay_ms( X ) usleep( X * 1000 )
  
  #ifdef __APPLE__
    // MacOS
    // TODO: find UART connection string for PN53X device on Mac OS X
    #define SERIAL_STRING ""
  #else
    // *BSD, Linux and others POSIX systems
    #define SERIAL_STRING "/dev/ttyUSB"
  #endif
#endif

#define BUFFER_LENGTH 256

#define SERIAL_DEFAULT_PORT_SPEED 115200

void pn532_uart_wakeup(const nfc_device_spec_t nds);
bool pn532_uart_check_communication(const nfc_device_spec_t nds);

nfc_device_desc_t *
pn532_uart_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t szN;

    if (!pn532_uart_list_devices (pndd, 1, &szN)) {
      DBG("%s", "pn532_uart_list_devices failed");
      return NULL;
    }

    if (szN == 0) {
      DBG("%s", "No device found");
      return NULL;
    }
  }

  return pndd;
}

bool
pn532_uart_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
  /** @note: Due to UART bus we can't know if its really a pn532 without
  * sending some PN53x commands. But using this way to probe devices, we can
  * have serious problem with other device on this bus */
#ifndef SERIAL_AUTOPROBE_ENABLED
  (void)pnddDevices;
  (void)szDevices;
  *pszDeviceFound = 0;
  DBG("%s", "Serial auto-probing have been disabled at compile time. Skipping autoprobe.");
  return false;
#else /* SERIAL_AUTOPROBE_ENABLED */
  *pszDeviceFound = 0;

  serial_port sp;
  char acPort[BUFFER_LENGTH];
  int iDevice;

  // I have no idea how MAC OS X deals with multiple devices, so a quick workaround
  for (iDevice=0; iDevice<DRIVERS_MAX_DEVICES; iDevice++)
  {
#ifdef __APPLE__
    strncpy(acPort,SERIAL_STRING, BUFFER_LENGTH - 1);
    acPort[BUFFER_LENGTH - 1] = '\0';
#else /* __APPLE__ */
    snprintf(acPort,BUFFER_LENGTH - 1,"%s%d",SERIAL_STRING,iDevice);
    acPort[BUFFER_LENGTH - 1] = '\0';
#endif /* __APPLE__ */
    sp = uart_open(acPort);
    DBG("Trying to find PN532 device on serial port: %s at %d bauds.",acPort, SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT))
    {
      // Serial port claimed but we need to check if a PN532_UART is connected.
      uart_set_speed(sp, SERIAL_DEFAULT_PORT_SPEED);
      // PN532 could be powered down, we need to wake it up before line testing.
      pn532_uart_wakeup((nfc_device_spec_t)sp);
      // Check communication using "Diagnose" command, with "Comunication test" (0x00)
      if(!pn532_uart_check_communication((nfc_device_spec_t)sp)) continue;
      uart_close(sp);

      snprintf(pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "PN532", acPort);
      pnddDevices[*pszDeviceFound].acDevice[DEVICE_NAME_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = PN532_UART_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup(acPort);
      pnddDevices[*pszDeviceFound].pcPort[BUFFER_LENGTH] = '\0';
      pnddDevices[*pszDeviceFound].uiSpeed = SERIAL_DEFAULT_PORT_SPEED;
      DBG("Device found: %s.", pnddDevices[*pszDeviceFound].acDevice);
      (*pszDeviceFound)++;

      // Test if we reach the maximum "wanted" devices
      if((*pszDeviceFound) >= szDevices) break;
    }
#ifdef DEBUG
    if (sp == INVALID_SERIAL_PORT) DBG("Invalid serial port: %s",acPort);
    if (sp == CLAIMED_SERIAL_PORT) DBG("Serial port already claimed: %s",acPort);
#endif /* DEBUG */
  }
#endif /* SERIAL_AUTOPROBE_ENABLED */
  return true;
}

nfc_device_t* pn532_uart_connect(const nfc_device_desc_t* pndd)
{
  serial_port sp;
  nfc_device_t* pnd = NULL;

  if( pndd == NULL ) {
    DBG("%s", "pn532_uart_connect() need an nfc_device_desc_t struct.");
    return NULL;
  } else {
    DBG("Attempt to connect to: %s at %d bauds.",pndd->pcPort, pndd->uiSpeed);
    sp = uart_open(pndd->pcPort);

    if (sp == INVALID_SERIAL_PORT) ERR("Invalid serial port: %s",pndd->pcPort);
    if (sp == CLAIMED_SERIAL_PORT) ERR("Serial port already claimed: %s",pndd->pcPort);
    if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT)) return NULL;

    uart_set_speed(sp, pndd->uiSpeed);
  }
  // PN532 could be powered down, we need to wake it up before line testing.
  pn532_uart_wakeup((nfc_device_spec_t)sp);
  // Check communication using "Diagnose" command, with "Comunication test" (0x00)
  if(!pn532_uart_check_communication((nfc_device_spec_t)sp)) return NULL;

  DBG("Successfully connected to: %s",pndd->pcPort);

  // We have a connection
  pnd = malloc(sizeof(nfc_device_t));
  strncpy(pnd->acName, pndd->acDevice, DEVICE_NAME_LENGTH - 1);
  pnd->acName[DEVICE_NAME_LENGTH - 1] = '\0';

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
  const byte_t pn53x_ack_frame[] = { 0x00,0x00,0xff,0x00,0xff,0x00 };
  const byte_t pn53x_nack_frame[] = { 0x00,0x00,0xff,0xff,0x00,0x00 };

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
  PRINT_HEX("TX", abtTxBuf,szTxLen+7);
#endif
  if (!uart_send((serial_port)nds,abtTxBuf,szTxLen+7)) {
    ERR("%s", "Unable to transmit data. (TX)");
    return false;
  }

  if (!uart_receive((serial_port)nds,abtRxBuf,&szRxBufLen)) {
    ERR("%s", "Unable to receive data. (RX)");
    return false;
  }

#ifdef DEBUG
  PRINT_HEX("RX", abtRxBuf,szRxBufLen);
#endif

  if(szRxBufLen >= sizeof(pn53x_ack_frame)) {

    // Check if PN53x reply ACK
    if(0!=memcmp(pn53x_ack_frame, abtRxBuf, sizeof(pn53x_ack_frame))) {
      DBG("%s", "PN53x doesn't respond ACK frame.");
      if (0==memcmp(pn53x_nack_frame, abtRxBuf, sizeof(pn53x_nack_frame))) {
        ERR("%s", "PN53x reply NACK frame.");
        // FIXME Handle NACK frame i.e. resend frame, PN53x doesn't received it correctly
      }
      return false;
    }

    szRxBufLen -= sizeof(pn53x_ack_frame);
    if(szRxBufLen) {
      memmove(abtRxBuf, abtRxBuf+sizeof(pn53x_ack_frame), szRxBufLen);
    }
  }

  if(szRxBufLen == 0) {
    // There was no more data than ACK frame, we need to wait next frame
    DBG("%s", "There was no more data than ACK frame, we need to wait next frame");
    while (!uart_receive((serial_port)nds,abtRxBuf,&szRxBufLen)) {
      delay_ms(10);
    }
  }

#ifdef DEBUG
  PRINT_HEX("RX", abtRxBuf,szRxBufLen);
#endif

  // When the answer should be ignored, just return a successful result
  if(pbtRx == NULL || pszRxLen == NULL) return true;

  // Only succeed when the result is at least 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if(szRxBufLen < 9) return false;

  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *pszRxLen = szRxBufLen - 9;
  memcpy(pbtRx, abtRxBuf+7, *pszRxLen);

  return true;
}

void
pn532_uart_wakeup(const nfc_device_spec_t nds)
{
  byte_t abtRx[BUFFER_LENGTH];
  size_t szRxLen;
  /** PN532C106 wakeup. */
  /** High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for PN532 being wakeup. */
  /** After the preamble we request the PN532C106 chip to switch to "normal" mode (SAM is not used) */
  const byte_t pncmd_pn532c106_wakeup_preamble[] = { 0x55,0x55,0x00,0x00,0x00,0x00,0x00,0xff,0x03,0xfd,0xd4,0x14,0x01,0x17,0x00,0x00,0xff,0x03,0xfd,0xd4,0x14,0x01,0x17,0x00 };
#ifdef DEBUG
  PRINT_HEX("TX", pncmd_pn532c106_wakeup_preamble,sizeof(pncmd_pn532c106_wakeup_preamble));
#endif
  uart_send((serial_port)nds, pncmd_pn532c106_wakeup_preamble, sizeof(pncmd_pn532c106_wakeup_preamble));
  if(uart_receive((serial_port)nds,abtRx,&szRxLen)) {
#ifdef DEBUG
    PRINT_HEX("RX", abtRx,szRxLen);
#endif
  }
}

bool
pn532_uart_check_communication(const nfc_device_spec_t nds)
{
  byte_t abtRx[BUFFER_LENGTH];
  size_t szRxLen;
  const byte_t attempted_result[] = { 0x00,0x00,0xff,0x00,0xff,0x00,0x00,0x00,0xff,0x09,0xf7,0xD5,0x01,0x00,'l','i','b','n','f','c',0xbc,0x00};

  /** To be sure that PN532 is alive, we have put a "Diagnose" command to execute a "Communication Line Test" */
  const byte_t pncmd_communication_test[] = { 0x00,0x00,0xff,0x09,0xf7,0xd4,0x00,0x00,'l','i','b','n','f','c',0xbe,0x00 };

#ifdef DEBUG
  PRINT_HEX("TX", pncmd_communication_test,sizeof(pncmd_communication_test));
#endif
  uart_send((serial_port)nds, pncmd_communication_test, sizeof(pncmd_communication_test));

  if(!uart_receive((serial_port)nds,abtRx,&szRxLen)) {
    return false;
  }
#ifdef DEBUG
  PRINT_HEX("RX", abtRx,szRxLen);
#endif

  if(0 != memcmp(abtRx,attempted_result,sizeof(attempted_result))) {
    DBG("%s", "Communication test failed, result doesn't match to attempted one.");
    return false;
  }
  return true;
}

#endif // DRIVER_PN532_UART_ENABLED

