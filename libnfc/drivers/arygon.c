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
 * @file arygon.c
 * @brief ARYGON readers driver
 * 
 * This driver can handle ARYGON readers that use UART as bus.
 * UART connection can be direct (host<->arygon_uc) or could be provided by internal USB to serial interface (e.g. host<->ftdi_chip<->arygon_uc)
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include "../drivers.h"

#include <stdio.h>
#include <string.h>

#include "arygon.h"

#include <nfc/nfc-messages.h>

// Bus
#include "uart.h"

#define BUFFER_LENGTH 256

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

#define SERIAL_DEFAULT_PORT_SPEED 9600

bool arygon_check_communication(const nfc_device_spec_t nds);

/**
 * @note ARYGON-ADRA (PN531): ???,n,8,1
 * @note ARYGON-ADRB (PN532): 9600,n,8,1
 * @note ARYGON-APDA (PN531): 9600,n,8,1
 * @note ARYGON-APDB1UA33N (PN532): 115200,n,8,1
 * @note ARYGON-APDB2UA33 (PN532 + ARYGON µC): 9600,n,8,1
 */
nfc_device_desc_t *
arygon_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t szN;

    if (!arygon_list_devices (pndd, 1, &szN)) {
      DBG("%s", "arygon_list_devices failed");
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
arygon_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
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
  const char** pcPorts = UNIX_SERIAL_PORT_DEVS;
  const char* pcPort;
  int iDevice = 0;

  while( pcPort = pcPorts[i++] ) {
    sp = uart_open(pcPort);
    DBG("Trying to find ARYGON device on serial port: %s at %d bauds.", pcPort, SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT))
    {
      uart_set_speed(sp, SERIAL_DEFAULT_PORT_SPEED);
      if(!arygon_check_communication((nfc_device_spec_t)sp)) continue;
      uart_close(sp);

      // ARYGON reader is found
      snprintf(pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "ARYGON", acPort);
      pnddDevices[*pszDeviceFound].acDevice[DEVICE_NAME_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = ARYGON_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup(pcPort);
      pnddDevices[*pszDeviceFound].uiSpeed = SERIAL_DEFAULT_PORT_SPEED;
      DBG("Device found: %s.", pnddDevices[*pszDeviceFound].acDevice);
      (*pszDeviceFound)++;

      // Test if we reach the maximum "wanted" devices
      if((*pszDeviceFound) >= szDevices) break;
    }
#ifdef DEBUG
    if (sp == INVALID_SERIAL_PORT) DBG("Invalid serial port: %s", pcPort);
    if (sp == CLAIMED_SERIAL_PORT) DBG("Serial port already claimed: %s", pcPort);
#endif /* DEBUG */
  }
#endif /* SERIAL_AUTOPROBE_ENABLED */
  return true;
}

nfc_device_t* arygon_connect(const nfc_device_desc_t* pndd)
{
  serial_port sp;
  nfc_device_t* pnd = NULL;

  DBG("Attempt to connect to: %s at %d bauds.",pndd->pcPort, pndd->uiSpeed);
  sp = uart_open(pndd->pcPort);

  if (sp == INVALID_SERIAL_PORT) ERR("Invalid serial port: %s",pndd->pcPort);
  if (sp == CLAIMED_SERIAL_PORT) ERR("Serial port already claimed: %s",pndd->pcPort);
  if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT)) return NULL;

  uart_set_speed(sp, pndd->uiSpeed);

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

void arygon_disconnect(nfc_device_t* pnd)
{
  uart_close((serial_port)pnd->nds);
  free(pnd);
}

bool arygon_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtTxBuf[BUFFER_LENGTH] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"
  byte_t abtRxBuf[BUFFER_LENGTH];
  size_t szRxBufLen = BUFFER_LENGTH;
  size_t szPos;

  const byte_t pn53x_ack_frame[] = { 0x00,0x00,0xff,0x00,0xff,0x00 };
  const byte_t pn53x_nack_frame[] = { 0x00,0x00,0xff,0xff,0x00,0x00 };

  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTxBuf[4] = szTxLen;
  // Packet length checksum
  abtTxBuf[5] = BUFFER_LENGTH - abtTxBuf[4];
  // Copy the PN53X command into the packet buffer
  memmove(abtTxBuf+6,pbtTx,szTxLen);

  // Calculate data payload checksum
  abtTxBuf[szTxLen+6] = 0;
  for(szPos=0; szPos < szTxLen; szPos++) 
  {
    abtTxBuf[szTxLen+6] -= abtTxBuf[szPos+6];
  }

  // End of stream marker
  abtTxBuf[szTxLen+7] = 0;

#ifdef DEBUG
  PRINT_HEX("TX", abtTxBuf,szTxLen+8);
#endif
  if (!uart_send((serial_port)nds,abtTxBuf,szTxLen+8)) {
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

//TODO Use tranceive function instead of raw uart send/receive for communication check.
bool
arygon_check_communication(const nfc_device_spec_t nds)
{
  byte_t abtRx[BUFFER_LENGTH];
  size_t szRxLen;
  const byte_t attempted_result[] = { 0x00,0x00,0xff,0x00,0xff,0x00,0x00,0x00,0xff,0x09,0xf7,0xD5,0x01,0x00,'l','i','b','n','f','c',0xbc,0x00};

  /** To be sure that PN532 is alive, we have put a "Diagnose" command to execute a "Communication Line Test" */
  const byte_t pncmd_communication_test[] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00,0x00,0xff,0x09,0xf7,0xd4,0x00,0x00,'l','i','b','n','f','c',0xbe,0x00 };

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

