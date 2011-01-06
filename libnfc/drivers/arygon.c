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
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "../drivers.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "arygon.h"

#include <nfc/nfc-messages.h>

// Bus
#include "uart.h"

#include <sys/param.h>

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

// TODO Move this one level up for libnfc-1.6
static const byte_t pn53x_ack_frame[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
// XXX It seems that sending arygon_ack_frame to cancel current command is not allowed by ARYGON µC (see arygon_ack())
// static const byte_t arygon_ack_frame[] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };

static const byte_t arygon_error_none[] = "FF000000\x0d\x0a";
static const byte_t arygon_error_incomplete_command[] = "FF0C0000\x0d\x0a";
static const byte_t arygon_error_unknown_mode[] = "FF060000\x0d\x0a";

// void    arygon_ack (const nfc_device_spec_t nds);
bool    arygon_reset_tama (const nfc_device_spec_t nds);
void    arygon_firmware (const nfc_device_spec_t nds, char * str);

bool    arygon_check_communication (const nfc_device_spec_t nds);

nfc_device_desc_t *
arygon_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t  szN;

    if (!arygon_list_devices (pndd, 1, &szN)) {
      DBG ("%s", "arygon_list_devices failed");
      free (pndd);
      return NULL;
    }

    if (szN == 0) {
      DBG ("%s", "No device found");
      free (pndd);
      return NULL;
    }
  }
  return pndd;
}

bool
arygon_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
{
  /** @note: Due to UART bus we can't know if its really a pn532 without
  * sending some PN53x commands. But using this way to probe devices, we can
  * have serious problem with other device on this bus */
#ifndef SERIAL_AUTOPROBE_ENABLED
  (void) pnddDevices;
  (void) szDevices;
  *pszDeviceFound = 0;
  DBG ("%s", "Serial auto-probing have been disabled at compile time. Skipping autoprobe.");
  return false;
#else /* SERIAL_AUTOPROBE_ENABLED */
  *pszDeviceFound = 0;

  serial_port sp;
  const char *pcPorts[] = DEFAULT_SERIAL_PORTS;
  const char *pcPort;
  int     iDevice = 0;

  while ((pcPort = pcPorts[iDevice++])) {
    sp = uart_open (pcPort);
    DBG ("Trying to find ARYGON device on serial port: %s at %d bauds.", pcPort, SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      uart_set_speed (sp, SERIAL_DEFAULT_PORT_SPEED);

      if (!arygon_reset_tama((nfc_device_spec_t) sp))
        continue;
      uart_close (sp);

      // ARYGON reader is found
      strncpy (pnddDevices[*pszDeviceFound].acDevice, "ARYGON", DEVICE_NAME_LENGTH - 1);
      pnddDevices[*pszDeviceFound].acDevice[DEVICE_NAME_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = ARYGON_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup (pcPort);
      pnddDevices[*pszDeviceFound].uiSpeed = SERIAL_DEFAULT_PORT_SPEED;
      DBG ("Device found: %s (%s)", pnddDevices[*pszDeviceFound].acDevice, pcPort);
      (*pszDeviceFound)++;

      // Test if we reach the maximum "wanted" devices
      if ((*pszDeviceFound) >= szDevices)
        break;
    }
#  ifdef DEBUG
    if (sp == INVALID_SERIAL_PORT)
      DBG ("Invalid serial port: %s", pcPort);
    if (sp == CLAIMED_SERIAL_PORT)
      DBG ("Serial port already claimed: %s", pcPort);
#  endif
       /* DEBUG */
  }
#endif /* SERIAL_AUTOPROBE_ENABLED */
  return true;
}

nfc_device_t *
arygon_connect (const nfc_device_desc_t * pndd)
{
  serial_port sp;
  nfc_device_t *pnd = NULL;

  DBG ("Attempt to connect to: %s at %d bauds.", pndd->pcPort, pndd->uiSpeed);
  sp = uart_open (pndd->pcPort);

  if (sp == INVALID_SERIAL_PORT)
    ERR ("Invalid serial port: %s", pndd->pcPort);
  if (sp == CLAIMED_SERIAL_PORT)
    ERR ("Serial port already claimed: %s", pndd->pcPort);
  if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT))
    return NULL;

  uart_set_speed (sp, pndd->uiSpeed);
  if (!arygon_reset_tama((nfc_device_spec_t) sp)) {
    return NULL;
  }

  DBG ("Successfully connected to: %s", pndd->pcPort);

  // We have a connection
  pnd = malloc (sizeof (nfc_device_t));
  char acFirmware[10];
  arygon_firmware((nfc_device_spec_t) sp, acFirmware);
  snprintf (pnd->acName, DEVICE_NAME_LENGTH - 1, "%s %s (%s)", pndd->acDevice, acFirmware, pndd->pcPort);
  pnd->acName[DEVICE_NAME_LENGTH - 1] = '\0';
  pnd->nc = NC_PN532;
  pnd->nds = (nfc_device_spec_t) sp;
  pnd->bActive = true;

  return pnd;
}

void
arygon_disconnect (nfc_device_t * pnd)
{
  uart_close ((serial_port) pnd->nds);
  free (pnd);
}

#define TX_BUFFER_LENGTH (300)
#define RX_BUFFER_LENGTH (PN53x_EXTENDED_FRAME_MAX_LEN + PN53x_EXTENDED_FRAME_OVERHEAD + sizeof(pn53x_ack_frame))
bool
arygon_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx, size_t * pszRx)
{
  byte_t  abtTxBuf[TX_BUFFER_LENGTH] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff };     // Every packet must start with "0x32 0x00 0x00 0xff"
  byte_t  abtRxBuf[RX_BUFFER_LENGTH];
  size_t  szRxBufLen;
  size_t  szReplyMaxLen = MIN(RX_BUFFER_LENGTH, *pszRx);
  size_t  szPos;
  int     res;

  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTxBuf[4] = szTx;
  // Packet length checksum
  abtTxBuf[5] = 256 - abtTxBuf[4];
  // Copy the PN53X command into the packet buffer
  memmove (abtTxBuf + 6, pbtTx, szTx);

  // Calculate data payload checksum
  abtTxBuf[szTx + 6] = 0;
  for (szPos = 0; szPos < szTx; szPos++) {
    abtTxBuf[szTx + 6] -= abtTxBuf[szPos + 6];
  }

  // End of stream marker
  abtTxBuf[szTx + 7] = 0;

#ifdef DEBUG
  PRINT_HEX ("TX", abtTxBuf, szTx + 8);
#endif
  res = uart_send ((serial_port) pnd->nds, abtTxBuf, szTx + 8);
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }
#ifdef DEBUG
  memset (abtRxBuf, 0x00, sizeof (abtRxBuf));
#endif
  szRxBufLen = szReplyMaxLen;
  res = uart_receive ((serial_port) pnd->nds, abtRxBuf, &szRxBufLen);
  if (res != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    pnd->iLastError = res;
    return false;
  }
#ifdef DEBUG
  PRINT_HEX ("RX", abtRxBuf, szRxBufLen);
#endif

  // WARN: UART is a per byte reception, so you usually receive ACK and next frame the same time
  if (!pn53x_check_ack_frame_callback (pnd, abtRxBuf, szRxBufLen))
    return false;

  szRxBufLen -= sizeof (pn53x_ack_frame);
  memmove (abtRxBuf, abtRxBuf + sizeof (pn53x_ack_frame), szRxBufLen);
  szReplyMaxLen -= sizeof (pn53x_ack_frame);

  if (szRxBufLen == 0) {
    do {
      delay_ms (10);
      szRxBufLen = szReplyMaxLen;
      res = uart_receive ((serial_port) pnd->nds, abtRxBuf, &szRxBufLen);
    } while (res != 0);
#ifdef DEBUG
    PRINT_HEX ("RX", abtRxBuf, szRxBufLen);
#endif
  }

  if (!pn53x_check_error_frame_callback (pnd, abtRxBuf, szRxBufLen))
    return false;

  // When the answer should be ignored, just return a successful result
  if (pbtRx == NULL || pszRx == NULL)
    return true;

  // Only succeed when the result is at least 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if (szRxBufLen < 9)
    return false;

  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *pszRx = szRxBufLen - 9;
  memcpy (pbtRx, abtRxBuf + 7, *pszRx);

  return true;
}

void
arygon_firmware (const nfc_device_spec_t nds, char * str)
{
  const byte_t arygon_firmware_version_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'v' }; 
  byte_t abtRx[RX_BUFFER_LENGTH];
  size_t szRx = 16;
  int res;

#ifdef DEBUG
  PRINT_HEX ("TX", arygon_firmware_version_cmd, sizeof (arygon_firmware_version_cmd));
#endif
  uart_send ((serial_port) nds, arygon_firmware_version_cmd, sizeof (arygon_firmware_version_cmd));

  res = uart_receive ((serial_port) nds, abtRx, &szRx);
  if (res != 0) {
    DBG ("Unable to retrieve ARYGON firmware version.");
    return;
  }
#ifdef DEBUG
  PRINT_HEX ("RX", abtRx, szRx);
#endif
  if ( 0 == memcmp (abtRx, arygon_error_none, 6)) {
    byte_t * p = abtRx + 6;
    unsigned int szData;
    sscanf ((const char*)p, "%02x%s", &szData, p);
    memcpy (str, p, szData);
    *(str + szData) = '\0';
  }
}

bool
arygon_reset_tama (const nfc_device_spec_t nds)
{
  const byte_t arygon_reset_tama_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'r' };
  byte_t abtRx[RX_BUFFER_LENGTH];
  size_t szRx = 10; // Attempted response is 10 bytes long
  int res;

  // Sometimes the first byte we send is not well-transmited (ie. a previously sent data on a wrong baud rate can put some junk in buffer)
#ifdef DEBUG
  PRINT_HEX ("TX", arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));
#endif
  uart_send ((serial_port) nds, arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));

  // Two reply are possible from ARYGON device: arygon_error_none (ie. in case the byte is well-sent)
  // or arygon_error_unknown_mode (ie. in case of the first byte was bad-transmitted)
  res = uart_receive ((serial_port) nds, abtRx, &szRx);
  if (res != 0) {
    DBG ("No reply to 'reset TAMA' command.");
    return false;
  }
#ifdef DEBUG
  PRINT_HEX ("RX", abtRx, szRx);
#endif
  if ( 0 == memcmp (abtRx, arygon_error_unknown_mode, sizeof (arygon_error_unknown_mode) - 1)) {
    // HACK Here we are... the first byte wasn't sent as expected, so we resend the same command
#ifdef DEBUG
      PRINT_HEX ("TX", arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));
#endif
      uart_send ((serial_port) nds, arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));
      res = uart_receive ((serial_port) nds, abtRx, &szRx);
      if (res != 0) {
        return false;
      }
#ifdef DEBUG
      PRINT_HEX ("RX", abtRx, szRx);
#endif
  }
  if (0 != memcmp (abtRx, arygon_error_none, sizeof (arygon_error_none) - 1)) {
    return false;
  }

  return true;
}

/*
void
arygon_ack (const nfc_device_spec_t nds)
{
  byte_t abtRx[BUFFER_LENGTH];
  size_t szRx;
#ifdef DEBUG
  PRINT_HEX ("TX", arygon_ack_frame, sizeof (arygon_ack_frame));
#endif
  uart_send ((serial_port) nds, arygon_ack_frame, sizeof (arygon_ack_frame));
  uart_receive ((serial_port) nds, abtRx, &szRx);
#ifdef DEBUG
  PRINT_HEX ("RX", abtRx, szRx);
#endif
  // ARYGON device will send an arygon_error_incomplete_command when sending an
  // ACK frame, and I (Romuald) don't know if the command is sent to PN or not
  if (0 != memcmp (abtRx, arygon_error_incomplete_command, sizeof (arygon_error_incomplete_command) - 1)) {
    return false;
  }
}
*/

bool
arygon_check_communication (const nfc_device_spec_t nds)
{
  byte_t  abtRx[RX_BUFFER_LENGTH];
  size_t  szRx;
  const byte_t attempted_result[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, // ACK
    0x00, 0x00, 0xff, 0x09, 0xf7, 0xd5, 0x01, 0x00, 'l', 'i', 'b', 'n', 'f', 'c', 0xbc, 0x00 }; // Reply
  int     res;

  /** To be sure that PN532 is alive, we have put a "Diagnose" command to execute a "Communication Line Test" */
  const byte_t pncmd_communication_test[] =
    { DEV_ARYGON_PROTOCOL_TAMA, // Header to passthrough front ARYGON µC (== directly talk to PN53x)
      0x00, 0x00, 0xff, 0x09, 0xf7, 0xd4, 0x00, 0x00, 'l', 'i', 'b', 'n', 'f', 'c', 0xbe, 0x00 };

#ifdef DEBUG
  PRINT_HEX ("TX", pncmd_communication_test, sizeof (pncmd_communication_test));
#endif
  res = uart_send ((serial_port) nds, pncmd_communication_test, sizeof (pncmd_communication_test));
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    return false;
  }

  res = uart_receive ((serial_port) nds, abtRx, &szRx);
  if (res != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    return false;
  }
#ifdef DEBUG
  PRINT_HEX ("RX", abtRx, szRx);
#endif

  if (0 != memcmp (abtRx, attempted_result, sizeof (attempted_result))) {
    DBG ("%s", "Communication test failed, result doesn't match to attempted one.");
    return false;
  }
  return true;
}
