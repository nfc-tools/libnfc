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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "../drivers.h"

#include <stdio.h>
#include <string.h>

#include "pn532_uart.h"

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>

// Bus
#include "uart.h"

#define SERIAL_DEFAULT_PORT_SPEED 115200

// TODO Move this one level up for libnfc-1.6
static const byte_t ack_frame[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };

void    pn532_uart_ack (const nfc_device_spec_t nds);
void    pn532_uart_wakeup (const nfc_device_spec_t nds);
bool    pn532_uart_check_communication (const nfc_device_spec_t nds, bool * success);

nfc_device_desc_t *
pn532_uart_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t  szN;

    if (!pn532_uart_list_devices (pndd, 1, &szN)) {
      DBG ("%s", "pn532_uart_list_devices failed");
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
pn532_uart_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
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
    DBG ("Trying to find PN532 device on serial port: %s at %d bauds.", pcPort, SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      bool    bComOk;
      // Serial port claimed but we need to check if a PN532_UART is connected.
      uart_set_speed (sp, SERIAL_DEFAULT_PORT_SPEED);
      // PN532 could be powered down, we need to wake it up before line testing.
      pn532_uart_wakeup ((nfc_device_spec_t) sp);
      // Check communication using "Diagnose" command, with "Communication test" (0x00)
      if (!pn532_uart_check_communication ((nfc_device_spec_t) sp, &bComOk))
        continue;
      if (!bComOk)
        continue;
      uart_close (sp);

      snprintf (pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "PN532", pcPort);
      pnddDevices[*pszDeviceFound].acDevice[DEVICE_NAME_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].pcDriver = PN532_UART_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup (pcPort);
      pnddDevices[*pszDeviceFound].uiSpeed = SERIAL_DEFAULT_PORT_SPEED;
      DBG ("Device found: %s.", pnddDevices[*pszDeviceFound].acDevice);
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
pn532_uart_connect (const nfc_device_desc_t * pndd)
{
  serial_port sp;
  nfc_device_t *pnd = NULL;
  bool    bComOk;

  DBG ("Attempt to connect to: %s at %d bauds.", pndd->pcPort, pndd->uiSpeed);
  sp = uart_open (pndd->pcPort);

  if (sp == INVALID_SERIAL_PORT)
    ERR ("Invalid serial port: %s", pndd->pcPort);
  if (sp == CLAIMED_SERIAL_PORT)
    ERR ("Serial port already claimed: %s", pndd->pcPort);
  if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT))
    return NULL;

  uart_set_speed (sp, pndd->uiSpeed);

  // PN532 could be powered down, we need to wake it up before line testing.
  pn532_uart_wakeup ((nfc_device_spec_t) sp);
  // Check communication using "Diagnose" command, with "Communication test" (0x00)
  if (!pn532_uart_check_communication ((nfc_device_spec_t) sp, &bComOk))
    return NULL;
  if (!bComOk)
    return NULL;

  DBG ("Successfully connected to: %s", pndd->pcPort);

  // We have a connection
  pnd = malloc (sizeof (nfc_device_t));
  strncpy (pnd->acName, pndd->acDevice, DEVICE_NAME_LENGTH - 1);
  pnd->acName[DEVICE_NAME_LENGTH - 1] = '\0';

  pnd->nc = NC_PN532;
  pnd->nds = (nfc_device_spec_t) sp;
  pnd->bActive = true;

  return pnd;
}

void
pn532_uart_disconnect (nfc_device_t * pnd)
{
  uart_close ((serial_port) pnd->nds);
  free (pnd);
}

#define TX_BUFFER_LEN (256)
#define RX_BUFFER_LEN (PN53x_EXTENDED_FRAME_MAX_LEN + PN53x_EXTENDED_FRAME_OVERHEAD)
bool
pn532_uart_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                       size_t * pszRx)
{
  byte_t  abtTxBuf[TX_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  byte_t  abtRxBuf[RX_BUFFER_LEN];
  size_t  szRxBufLen = MIN( RX_BUFFER_LEN, *pszRx );
  size_t  szPos;
  int     res;

  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTxBuf[3] = szTx;
  // Packet length checksum
  abtTxBuf[4] = 256 - abtTxBuf[3];
  // Copy the PN53X command into the packet buffer
  memmove (abtTxBuf + 5, pbtTx, szTx);

  // Calculate data payload checksum
  abtTxBuf[szTx + 5] = 0;
  for (szPos = 0; szPos < szTx; szPos++) {
    abtTxBuf[szTx + 5] -= abtTxBuf[szPos + 5];
  }

  // End of stream marker
  abtTxBuf[szTx + 6] = 0;

#ifdef DEBUG
  PRINT_HEX ("TX", abtTxBuf, szTx + 7);
#endif
  res = uart_send ((serial_port) pnd->nds, abtTxBuf, szTx + 7);
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }

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
  szRxBufLen -= sizeof (ack_frame);
  memmove (abtRxBuf, abtRxBuf + sizeof (ack_frame), szRxBufLen);

  if (szRxBufLen == 0) {
    szRxBufLen = RX_BUFFER_LEN;
    do {
      delay_ms (10);
      res = uart_receive ((serial_port) pnd->nds, abtRxBuf, &szRxBufLen);
    } while (res != 0);
#ifdef DEBUG
    PRINT_HEX ("RX", abtRxBuf, szRxBufLen);
#endif
  }

#ifdef DEBUG
  PRINT_HEX ("TX", ack_frame, sizeof(ack_frame));
#endif
  res = uart_send ((serial_port) pnd->nds, ack_frame, sizeof(ack_frame));
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }

  if (!pn53x_check_error_frame_callback (pnd, abtRxBuf, szRxBufLen))
    return false;

  // When the answer should be ignored, just return a successful result
  if (pbtRx == NULL || pszRx == NULL)
    return true;

  // Only succeed when the result is at least 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if (szRxBufLen < 9) {
    pnd->iLastError = DEINVAL;
    return false;
  }
  // Remove the preceding and appending bytes 00 00 ff 00 ff 00 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *pszRx = szRxBufLen - 9;
  memcpy (pbtRx, abtRxBuf + 7, *pszRx);

  return true;
}

void
pn532_uart_ack (const nfc_device_spec_t nds)
{
#ifdef DEBUG
  PRINT_HEX ("TX", ack_frame, sizeof (ack_frame));
#endif
  uart_send ((serial_port) nds, ack_frame, sizeof (ack_frame));
}

bool
pn532_uart_wait_for_ack(const nfc_device_spec_t nds)
{
  byte_t  abtRx[RX_BUFFER_LEN];
  size_t  szRx = sizeof(ack_frame);
  if (0 == uart_receive ((serial_port) nds, abtRx, &szRx)) {
#ifdef DEBUG           
    PRINT_HEX ("RX", abtRx, szRx);
#endif
  } else {
    ERR ("No ACK.");
    return false;
  }
  if (0 != memcmp (ack_frame, abtRx, szRx))
    return false;
  return true;
}

#define PN53X_RX_OVERHEAD 6
void
pn532_uart_wakeup (const nfc_device_spec_t nds)
{
  byte_t  abtRx[RX_BUFFER_LEN];
  size_t  szRx = PN53x_NORMAL_FRAME_OVERHEAD + 2;
  /** PN532C106 wakeup. */
  /** High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for PN532 being wakeup. */
  /** After the preamble we request the PN532C106 chip to switch to "normal" mode (SAM is not used) */
  const byte_t pncmd_pn532c106_wakeup_preamble[] = 
    { 0x55, 0x55, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xff, 0x03, 0xfd, 0xd4, 0x14, 0x01, 0x17, 0x00 }; // Here we send a SAMConfiguration command (Normal mode, the SAM is not used; this is the default mode)
#ifdef DEBUG
  PRINT_HEX ("TX", pncmd_pn532c106_wakeup_preamble, sizeof (pncmd_pn532c106_wakeup_preamble));
#endif
  uart_send ((serial_port) nds, pncmd_pn532c106_wakeup_preamble, sizeof (pncmd_pn532c106_wakeup_preamble));

  pn532_uart_wait_for_ack(nds);

  if (0 == uart_receive ((serial_port) nds, abtRx, &szRx)) {
#ifdef DEBUG
    PRINT_HEX ("RX", abtRx, szRx);
#endif
  } else {
    ERR ("Unable to wakeup the PN532.");
  }
}

bool
pn532_uart_check_communication (const nfc_device_spec_t nds, bool * success)
{
  byte_t  abtRx[RX_BUFFER_LEN];
  const byte_t attempted_result[] =
    { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x09, 0xf7, 0xD5, 0x01, 0x00, 'l', 'i', 'b', 'n', 'f', 'c',
0xbc, 0x00 };
  size_t  szRx = sizeof(attempted_result);
  int     res;

  /** To be sure that PN532 is alive, we have put a "Diagnose" command to execute a "Communication Line Test" */
  const byte_t pncmd_communication_test[] =
    { 0x00, 0x00, 0xff, 0x09, 0xf7, 0xd4, 0x00, 0x00, 'l', 'i', 'b', 'n', 'f', 'c', 0xbe, 0x00 };

  *success = false;

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

  if (0 == memcmp (abtRx, attempted_result, sizeof (attempted_result)))
    *success = true;

  return true;
}
