/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Roel Verdult, Romuald Conty
 * Copyright (C) 2011, Romuald Conty, Romain Tartière
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

/* vim: set ts=2 sw=2 et: */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "pn532_uart.h"

#include <stdio.h>
#include <string.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "uart.h"

#define PN532_UART_DEFAULT_SPEED 115200
#define PN532_UART_DRIVER_NAME "PN532_UART"

// TODO Move this one level up for libnfc-1.6
static const byte_t ack_frame[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };

int     pn532_uart_ack (nfc_device_t * pnd);
// void    pn532_uart_wakeup (const nfc_device_spec_t nds);

const struct pn53x_io pn532_uart_io;

struct pn532_uart_data {
  serial_port port;
};
  
#define DRIVER_DATA(pnd) ((struct pn532_uart_data*)(pnd->driver_data))

bool
pn532_uart_probe (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
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
  char **pcPorts = uart_list_ports ();
  const char *pcPort;
  int     iDevice = 0;

  while ((pcPort = pcPorts[iDevice++])) {
    sp = uart_open (pcPort);
    DBG ("Trying to find PN532 device on serial port: %s at %d bauds.", pcPort, PN532_UART_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      // Serial port claimed but we need to check if a PN532_UART is connected.
      uart_set_speed (sp, PN532_UART_DEFAULT_SPEED);

      nfc_device_t *pnd = nfc_device_new ();
      pnd->iLastError = 0;
      pnd->driver = &pn532_uart_driver;
      pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
      DRIVER_DATA (pnd)->port = sp;
      pnd->chip_data = malloc(sizeof(struct pn53x_data));
      CHIP_DATA (pnd)->type = PN532;
      CHIP_DATA (pnd)->state = SLEEP;
      CHIP_DATA (pnd)->io = &pn532_uart_io;

      // Check communication using "Diagnose" command, with "Communication test" (0x00)
      bool res = pn53x_check_communication (pnd);
      if(!res) {
        nfc_perror (pnd, "pn53x_check_communication");
      }
      nfc_device_free (pnd);
      uart_close (sp);
      if(!res) {
        continue;
      }

      snprintf (pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "PN532", pcPort);
      pnddDevices[*pszDeviceFound].pcDriver = PN532_UART_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].pcPort = strdup (pcPort);
      pnddDevices[*pszDeviceFound].uiSpeed = PN532_UART_DEFAULT_SPEED;
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
pn532_uart_connect (const nfc_device_desc_t * pndd)
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
  pnd->acName[DEVICE_NAME_LENGTH - 1] = '\0';

  pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
  DRIVER_DATA(pnd)->port = sp;
  pnd->chip_data = malloc(sizeof(struct pn53x_data));
  CHIP_DATA(pnd)->type = PN532;
  CHIP_DATA(pnd)->state = SLEEP;
  CHIP_DATA(pnd)->io = &pn532_uart_io;
  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 48;
  pnd->driver = &pn532_uart_driver;

  // Check communication using "Diagnose" command, with "Communication test" (0x00)
  if (!pn53x_check_communication (pnd)) {
    nfc_perror (pnd, "pn53x_check_communication");
    pn532_uart_disconnect(pnd);
    return NULL;
  }

  pn53x_init(pnd);
  return pnd;
}

void
pn532_uart_disconnect (nfc_device_t * pnd)
{
  uart_close (DRIVER_DATA(pnd)->port);
  nfc_device_free (pnd);
}

#define PN532_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
bool
pn532_uart_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData)
{
  if (CHIP_DATA(pnd)->state == SLEEP) {
    /** PN532C106 wakeup. */
    /** High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for PN532 being wakeup. */
    const byte_t pn532_wakeup_preamble[] = { 0x55, 0x55, 0x00, 0x00, 0x00 };
    uart_send (DRIVER_DATA(pnd)->port, pn532_wakeup_preamble, sizeof (pn532_wakeup_preamble));
    CHIP_DATA(pnd)->state = NORMAL; // PN532 should now be awake
    // According to PN532 application note, C106 appendix: to go out Low Vbat mode and enter in normal mode we need to send a SAMConfiguration command
    if (!pn53x_SAMConfiguration (pnd, 0x01)) {
      return false;
    }
  }

  byte_t  abtFrame[PN532_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  CHIP_DATA (pnd)->ui8LastCommand = pbtData[0];
  size_t szFrame = 0;

  if (!pn53x_build_frame (abtFrame, &szFrame, pbtData, szData)) {
    pnd->iLastError = DEINVAL;
    return false;
  }

  int res = uart_send (DRIVER_DATA(pnd)->port, abtFrame, szFrame);
  if (res != 0) {
    ERR ("%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }

  byte_t abtRxBuf[6];
  res = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 6, 0);
  if (res != 0) {
    ERR ("%s", "Unable to read ACK");
    pnd->iLastError = res;
    return false;
  }

  if (pn53x_check_ack_frame (pnd, abtRxBuf, sizeof(abtRxBuf))) {
    CHIP_DATA(pnd)->state = EXECUTE;
  } else {
    return false;
  }
  return true;
}

int
pn532_uart_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szDataLen)
{
  byte_t  abtRxBuf[5];
  size_t len;
  int abort_fd = 0;

  switch (CHIP_DATA (pnd)->ui8LastCommand) {
  case InAutoPoll:
  case TgInitAsTarget:
  case TgGetData:
    abort_fd = pnd->iAbortFds[1];
    break;
  default:
    break;
  }

  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 5, abort_fd);

  if (abort_fd && (DEABORT == pnd->iLastError)) {
    pn532_uart_ack (pnd);
    return -1;
  }

  if (pnd->iLastError != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
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
    uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0);
    ERR ("%s", "Application level error detected");
    pnd->iLastError = DEISERRFRAME;
    return -1;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0);

    // (abtRxBuf[0] << 8) + abtRxBuf[1] (LEN) include TFI + (CC+1)
    len = (abtRxBuf[0] << 8) + abtRxBuf[1] - 2;
    if (((abtRxBuf[0] + abtRxBuf[1] + abtRxBuf[2]) % 256) != 0) {
      // TODO: Retry
      ERR ("%s", "Length checksum mismatch");
      pnd->iLastError = DEIO;
      return -1;
    }
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
  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0);
  if (pnd->iLastError != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    return -1;
  }

  if (abtRxBuf[0] != 0xD5) {
    ERR ("%s", "TFI Mismatch");
    pnd->iLastError = DEIO;
    return -1;
  }

  if (abtRxBuf[1] != CHIP_DATA (pnd)->ui8LastCommand + 1) {
    ERR ("%s", "Command Code verification failed");
    pnd->iLastError = DEIO;
    return -1;
  }

  if (len) {
    pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, pbtData, len, 0);
    if (pnd->iLastError != 0) {
      ERR ("%s", "Unable to receive data. (RX)");
      return -1;
    }
  }

  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0);
  if (pnd->iLastError != 0) {
    ERR ("%s", "Unable to receive data. (RX)");
    return -1;
  }

  byte_t btDCS = (256 - 0xD5);
  btDCS -= CHIP_DATA (pnd)->ui8LastCommand + 1;
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
  CHIP_DATA(pnd)->state = NORMAL;
  return len;
}

int
pn532_uart_ack (nfc_device_t * pnd)
{
  CHIP_DATA(pnd)->state = NORMAL;
  return (0 == uart_send (DRIVER_DATA(pnd)->port, ack_frame, sizeof (ack_frame))) ? 0 : -1;
}

const struct pn53x_io pn532_uart_io = {
  .send       = pn532_uart_send,
  .receive    = pn532_uart_receive,
};

const struct nfc_driver_t pn532_uart_driver = {
  .name       = PN532_UART_DRIVER_NAME,
  .probe      = pn532_uart_probe,
  .connect    = pn532_uart_connect,
  .disconnect = pn532_uart_disconnect,
  .strerror   = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
  .initiator_poll_targets           = pn53x_initiator_poll_targets,
  .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
  .initiator_deselect_target        = pn53x_initiator_deselect_target,
  .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
  .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
  .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
  .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,

  .target_init           = pn53x_target_init,
  .target_send_bytes     = pn53x_target_send_bytes,
  .target_receive_bytes  = pn53x_target_receive_bytes,
  .target_send_bits      = pn53x_target_send_bits,
  .target_receive_bits   = pn53x_target_receive_bits,

  .configure  = pn53x_configure,
};

