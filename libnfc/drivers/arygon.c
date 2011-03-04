/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romuald Conty
 * Copyright (C) 2011, Romain Tartière, Romuald Conty
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

#include "arygon.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>

#include "libnfc/drivers.h"
#include "libnfc/nfc-internal.h"
#include "libnfc/chips/pn53x.h"
#include "libnfc/chips/pn53x-internal.h"
#include "uart.h"

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

#define ARYGON_SERIAL_DEFAULT_PORT_SPEED 9600

struct arygon_data {
  serial_port port;
};

// XXX It seems that sending arygon_ack_frame to cancel current command is not allowed by ARYGON µC (see arygon_ack())
// static const byte_t arygon_ack_frame[] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };

static const byte_t arygon_error_none[] = "FF000000\x0d\x0a";
static const byte_t arygon_error_incomplete_command[] = "FF0C0000\x0d\x0a";
static const byte_t arygon_error_unknown_mode[] = "FF060000\x0d\x0a";

// void    arygon_ack (const nfc_device_spec_t nds);
bool    arygon_reset_tama (nfc_device_t * pnd);
void    arygon_firmware (nfc_device_t * pnd, char * str);
bool    arygon_check_communication (nfc_device_t * pnd);

bool
arygon_probe (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
{
  /** @note: Due to UART bus we can't know if its really an ARYGON without
  * sending some commands. But using this way to probe devices, we can
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
  char **pcPorts = uart_list_ports ();
  const char *pcPort;
  int     iDevice = 0;

  while ((pcPort = pcPorts[iDevice++])) {
    sp = uart_open (pcPort);
    DBG ("Trying to find ARYGON device on serial port: %s at %d bauds.", pcPort, ARYGON_SERIAL_DEFAULT_PORT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      uart_set_speed (sp, ARYGON_SERIAL_DEFAULT_PORT_SPEED);

      nfc_device_t nd;
      nd.driver = &arygon_driver;
      nd.driver_data = malloc(sizeof(struct arygon_data));
      ((struct arygon_data*)(nd.driver_data))->port = sp;
      nd.chip_data = malloc(sizeof(struct pn53x_data));
      ((struct pn53x_data*)(nd.chip_data))->type = PN532;
      ((struct pn53x_data*)(nd.chip_data))->state = NORMAL;

      bool res = arygon_reset_tama(&nd);
      free(nd.driver_data);
      free(nd.chip_data);
      uart_close (sp);
      if(!res)
        continue;

      // ARYGON reader is found
      snprintf (pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "Arygon", pcPort);
      pnddDevices[*pszDeviceFound].pcDriver = ARYGON_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup (pcPort);
      pnddDevices[*pszDeviceFound].uiSpeed = ARYGON_SERIAL_DEFAULT_PORT_SPEED;
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
  free (pcPorts);
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

  // We have a connection
  pnd = malloc (sizeof (nfc_device_t));
  strncpy (pnd->acName, pndd->acDevice, DEVICE_NAME_LENGTH - 1);

  pnd->driver_data = malloc(sizeof(struct arygon_data));
  ((struct arygon_data*)(pnd->driver_data))->port = sp;
  pnd->chip_data = malloc(sizeof(struct pn53x_data));

  // The PN53x chip connected to ARYGON MCU doesn't seems to be in SLEEP mode
  ((struct pn53x_data*)(pnd->chip_data))->state = NORMAL;
  pnd->driver = &arygon_driver;

  // Check communication using "Reset TAMA" command
  if (!arygon_reset_tama(pnd)) {
    free(pnd->driver_data);
    free(pnd->chip_data);
    free(pnd);
    return NULL;
  }

  pn53x_init(pnd);
  return pnd;
}

void
arygon_disconnect (nfc_device_t * pnd)
{
  uart_close (((struct arygon_data*)(pnd->driver_data))->port);
  free(pnd->driver_data);
  free(pnd->chip_data);
  free (pnd);
}

#define ARYGON_TX_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD + 1)
#define ARYGON_RX_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
bool
arygon_tama_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData)
{
  byte_t abtFrame[ARYGON_TX_BUFFER_LEN] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff };     // Every packet must start with "0x32 0x00 0x00 0xff"

  pnd->iLastCommand = pbtData[0];
  size_t szFrame = 0;
  pn53x_build_frame (abtFrame + 1, &szFrame, pbtData, szData);

  int res = uart_send (((struct arygon_data*)(pnd->driver_data))->port, abtFrame, szFrame + 1);
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }

  byte_t abtRxBuf[6];
  res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRxBuf, 6);
  if (res != 0) {
    ERR ("%s", "Unable to read ACK");
    pnd->iLastError = res;
    return false;
  }

  if (pn53x_check_ack_frame (pnd, abtRxBuf, sizeof(abtRxBuf))) {
    ((struct pn53x_data*)(pnd->chip_data))->state = EXECUTE;
  } else if (0 == memcmp(arygon_error_unknown_mode, abtRxBuf, sizeof(abtRxBuf))) {
    ERR( "Bad frame format." );
    return false;
  } else {
    return false;
  }
  return true;
}

int
arygon_tama_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szDataLen)
{
  byte_t  abtRxBuf[5];
  size_t len;

  int res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRxBuf, 5);
  if (res != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    pnd->iLastError = res;
    return -1;
  }

  const byte_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };
  if (0 != (memcmp (abtRxBuf, pn53x_preamble, 3))) {
    ERR ("%s", "Frame preamble+start code mismatch");
    pnd->iLastError = DEIO;
    return -1;
  }

  if ((0x01 == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Error frame
    uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRxBuf, 3);
    ERR ("%s", "Application level error detected");
    pnd->iLastError = DEISERRFRAME;
    return -1;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    // FIXME: Code this
    abort ();
  } else {
    // Normal frame
    if (256 != (abtRxBuf[3] + abtRxBuf[4])) {
      // TODO: Retry
      ERR ("%s", "Length checksum mismatch");
      pnd->iLastError = DEIO;
      return -1;
    }

    // abtRxBuf[3] (LEN) include TFI + (CC+1)
    len = abtRxBuf[3] - 2;
  }

  if (len > szDataLen) {
    ERR ("Unable to receive data: buffer too small. (szDataLen: %zu, len: %zu)", szDataLen, len);
    pnd->iLastError = DEIO;
    return -1;
  }

  // TFI + PD0 (CC+1)
  res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRxBuf, 2);
  if (res != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    pnd->iLastError = res;
    return -1;
  }

  if (abtRxBuf[0] != 0xD5) {
    ERR ("%s", "TFI Mismatch");
    pnd->iLastError = DEIO;
    return -1;
  }

  if (abtRxBuf[1] != pnd->iLastCommand + 1) {
    ERR ("%s", "Command Code verification failed");
    pnd->iLastError = DEIO;
    return -1;
  }

  if (len) {
    res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, pbtData, len);
    if (res != 0) {
      ERR ("%s", "Unable to receive data. (RX)");
      pnd->iLastError = res;
      return -1;
    }
  }

  res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRxBuf, 2);
  if (res != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    pnd->iLastError = res;
    return -1;
  }

  byte_t btDCS = (256 - 0xD5);
  btDCS -= pnd->iLastCommand + 1;
  for (size_t szPos = 0; szPos < len; szPos++) {
    btDCS -= pbtData[szPos];
  }

  if (btDCS != abtRxBuf[0]) {
    ERR ("%s", "Data checksum mismatch");
    pnd->iLastError = DEIO;
    return -1;
  }

  if (0x00 != abtRxBuf[1]) {
    ERR ("%s", "Frame postamble mismatch");
    pnd->iLastError = DEIO;
    return -1;
  }
  ((struct pn53x_data*)(pnd->chip_data))->state = NORMAL;
  return len;
}

void
arygon_firmware (nfc_device_t * pnd, char * str)
{
  const byte_t arygon_firmware_version_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'v' }; 
  byte_t abtRx[16];
  size_t szRx = sizeof(abtRx);


  int res = uart_send (((struct arygon_data*)(pnd->driver_data))->port, arygon_firmware_version_cmd, sizeof (arygon_firmware_version_cmd));
  if (res != 0) {
    DBG ("Unable to send ARYGON firmware command.");
    return;
  }
  res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRx, szRx);
  if (res != 0) {
    DBG ("Unable to retrieve ARYGON firmware version.");
    return;
  }

  if ( 0 == memcmp (abtRx, arygon_error_none, 6)) {
    byte_t * p = abtRx + 6;
    unsigned int szData;
    sscanf ((const char*)p, "%02x%s", &szData, p);
    memcpy (str, p, szData);
    *(str + szData) = '\0';
  }
}

bool
arygon_reset_tama (nfc_device_t * pnd)
{
  const byte_t arygon_reset_tama_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'r' };
  byte_t abtRx[10]; // Attempted response is 10 bytes long
  size_t szRx = sizeof(abtRx);
  int res;

  uart_send (((struct arygon_data*)(pnd->driver_data))->port, arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));

  // Two reply are possible from ARYGON device: arygon_error_none (ie. in case the byte is well-sent)
  // or arygon_error_unknown_mode (ie. in case of the first byte was bad-transmitted)
  res = uart_receive (((struct arygon_data*)(pnd->driver_data))->port, abtRx, szRx);
  if (res != 0) {
    DBG ("No reply to 'reset TAMA' command.");
    return false;
  }
#if 0
  if ( 0 == memcmp (abtRx, arygon_error_unknown_mode, sizeof (arygon_error_unknown_mode))) {
    // HACK Here we are... the first byte wasn't sent as expected, so we resend the same command
      uart_send ((serial_port) nds, arygon_reset_tama_cmd, sizeof (arygon_reset_tama_cmd));
      res = uart_receive ((serial_port) nds, abtRx, &szRx);
      if (res != 0) {
        return false;
      }
  }
#endif
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
  size_t szRx = sizeof(abtRx);
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


const struct nfc_driver_t arygon_driver = {
  .name       = ARYGON_DRIVER_NAME,
  .probe      = arygon_probe,
  .connect    = arygon_connect,
  .send       = arygon_tama_send,
  .receive    = arygon_tama_receive,
  .disconnect = arygon_disconnect,
  .strerror   = pn53x_strerror,
};

