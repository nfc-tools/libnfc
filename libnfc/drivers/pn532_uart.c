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
#include <unistd.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "uart.h"

#define PN532_UART_DEFAULT_SPEED 115200
#define PN532_UART_DRIVER_NAME "PN532_UART"
#define LOG_CATEGORY "libnfc.driver.pn532_uart"

int     pn532_uart_ack (nfc_device_t * pnd);
int     pn532_uart_wakeup (nfc_device_t * pnd);

const struct pn53x_io pn532_uart_io;

struct pn532_uart_data {
  serial_port port;
#ifndef WIN32
  int     iAbortFds[2];
#else
  volatile bool abort_flag;
#endif
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
  log_put (LOG_CATEGORY, NFC_PRIORITY_INFO, "Serial auto-probing have been disabled at compile time. Skipping autoprobe.");
  return false;
#else /* SERIAL_AUTOPROBE_ENABLED */
  *pszDeviceFound = 0;

  serial_port sp;
  char **acPorts = uart_list_ports ();
  const char *acPort;
  int     iDevice = 0;

  while ((acPort = acPorts[iDevice++])) {
    sp = uart_open (acPort);
    log_put (LOG_CATEGORY, NFC_PRIORITY_TRACE, "Trying to find PN532 device on serial port: %s at %d bauds.", acPort, PN532_UART_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      // We need to flush input to be sure first reply does not comes from older byte transceive
      uart_flush_input (sp);
      // Serial port claimed but we need to check if a PN532_UART is connected.
      uart_set_speed (sp, PN532_UART_DEFAULT_SPEED);

      nfc_device_t *pnd = nfc_device_new ();
      pnd->driver = &pn532_uart_driver;
      pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
      DRIVER_DATA (pnd)->port = sp;

      // Alloc and init chip's data
      pn53x_data_new (pnd, &pn532_uart_io);
      // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
      CHIP_DATA (pnd)->type = PN532;
      // This device starts in LowVBat power mode
      CHIP_DATA (pnd)->power_mode = LOWVBAT;

#ifndef WIN32
      // pipe-based abort mecanism
      pipe (DRIVER_DATA (pnd)->iAbortFds);
#else
      DRIVER_DATA (pnd)->abort_flag = false;
#endif

      // Check communication using "Diagnose" command, with "Communication test" (0x00)
      bool res = pn53x_check_communication (pnd);
      if(!res) {
        nfc_perror (pnd, "pn53x_check_communication");
      }
      pn53x_data_free (pnd);
      nfc_device_free (pnd);
      uart_close (sp);
      if(!res) {
        continue;
      }

      snprintf (pnddDevices[*pszDeviceFound].acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", "PN532", acPort);
      pnddDevices[*pszDeviceFound].pcDriver = PN532_UART_DRIVER_NAME;
      strncpy (pnddDevices[*pszDeviceFound].acPort, acPort, DEVICE_PORT_LENGTH - 1); pnddDevices[*pszDeviceFound].acPort[DEVICE_PORT_LENGTH - 1] = '\0';
      pnddDevices[*pszDeviceFound].uiSpeed = PN532_UART_DEFAULT_SPEED;
      (*pszDeviceFound)++;

      // Test if we reach the maximum "wanted" devices
      if ((*pszDeviceFound) >= szDevices)
        break;
    }
  }

  iDevice = 0;
  while ((acPort = acPorts[iDevice++])) {
    free ((void*)acPort);
  }
  free (acPorts);
#endif /* SERIAL_AUTOPROBE_ENABLED */
  return true;
}

nfc_device_t *
pn532_uart_connect (const nfc_device_desc_t * pndd)
{
  serial_port sp;
  nfc_device_t *pnd = NULL;

  log_put (LOG_CATEGORY, NFC_PRIORITY_TRACE, "Attempt to connect to: %s at %d bauds.", pndd->acPort, pndd->uiSpeed);
  sp = uart_open (pndd->acPort);

  if (sp == INVALID_SERIAL_PORT)
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "Invalid serial port: %s", pndd->acPort);
  if (sp == CLAIMED_SERIAL_PORT)
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "Serial port already claimed: %s", pndd->acPort);
  if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT))
    return NULL;

  // We need to flush input to be sure first reply does not comes from older byte transceive
  uart_flush_input (sp);
  uart_set_speed (sp, pndd->uiSpeed);

  // We have a connection
  pnd = nfc_device_new ();
  strncpy (pnd->acName, pndd->acDevice, DEVICE_NAME_LENGTH - 1);
  pnd->acName[DEVICE_NAME_LENGTH - 1] = '\0';

  pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
  DRIVER_DATA(pnd)->port = sp;
  // Alloc and init chip's data
  pn53x_data_new (pnd, &pn532_uart_io);
  // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
  CHIP_DATA (pnd)->type = PN532;
  // This device starts in LowVBat mode
  CHIP_DATA(pnd)->power_mode = LOWVBAT;

  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 48;
  pnd->driver = &pn532_uart_driver;

#ifndef WIN32
  // pipe-based abort mecanism
  pipe (DRIVER_DATA (pnd)->iAbortFds);
#else
  DRIVER_DATA (pnd)->abort_flag = false;
#endif

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
  // Release UART port
  uart_close (DRIVER_DATA(pnd)->port);

#ifndef WIN32
  // Release file descriptors used for abort mecanism
  close (DRIVER_DATA (pnd)->iAbortFds[0]);
  close (DRIVER_DATA (pnd)->iAbortFds[1]);
#endif

  pn53x_data_free (pnd);
  nfc_device_free (pnd);
}

int
pn532_uart_wakeup (nfc_device_t * pnd)
{
  /* High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for PN532 being wakeup. */
  const byte_t pn532_wakeup_preamble[] = { 0x55, 0x55, 0x00, 0x00, 0x00 };
  int res = uart_send (DRIVER_DATA(pnd)->port, pn532_wakeup_preamble, sizeof (pn532_wakeup_preamble), NULL);
  CHIP_DATA(pnd)->power_mode = NORMAL; // PN532 should now be awake
  return res;
}

#define PN532_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
bool
pn532_uart_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData, struct timeval *timeout)
{
  // Before sending anything, we need to discard from any junk bytes
  uart_flush_input (DRIVER_DATA(pnd)->port);

  switch (CHIP_DATA(pnd)->power_mode) {
    case LOWVBAT: {
      /** PN532C106 wakeup. */
      if (-1 == pn532_uart_wakeup(pnd)) {
        return false;
      }
      // According to PN532 application note, C106 appendix: to go out Low Vbat mode and enter in normal mode we need to send a SAMConfiguration command
      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      if (!pn53x_SAMConfiguration (pnd, 0x01, &tv)) {
        return false;
      }
    }
    break;
    case POWERDOWN: {
      if (-1 == pn532_uart_wakeup(pnd)) {
        return false;
      }
    }
    break;
    case NORMAL:
      // Nothing to do :)
    break;
  };

  byte_t  abtFrame[PN532_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  size_t szFrame = 0;

  if (!pn53x_build_frame (abtFrame, &szFrame, pbtData, szData)) {
    pnd->iLastError = EINVALARG;
    return false;
  }

  int res = uart_send (DRIVER_DATA(pnd)->port, abtFrame, szFrame, timeout);
  if (res != 0) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->iLastError = res;
    return false;
  }

  byte_t abtRxBuf[6];
  res = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 6, 0, timeout);
  if (res != 0) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to read ACK");
    pnd->iLastError = res;
    return false;
  }

  if (pn53x_check_ack_frame (pnd, abtRxBuf, sizeof(abtRxBuf))) {
    // The PN53x is running the sent command
  } else {
    return false;
  }
  return true;
}

int
pn532_uart_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szDataLen, struct timeval *timeout)
{
  byte_t  abtRxBuf[5];
  size_t len;
  void * abort_p = NULL;

#ifndef WIN32
  abort_p = &(DRIVER_DATA (pnd)->iAbortFds[1]);
#else
  abort_p = (void*)&(DRIVER_DATA (pnd)->abort_flag);
#endif

  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 5, abort_p, timeout);

  if (abort_p && (EOPABORT == pnd->iLastError)) {
    pn532_uart_ack (pnd);
    return -1;
  }

  if (pnd->iLastError != 0) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return -1;
  }

  const byte_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };
  if (0 != (memcmp (abtRxBuf, pn53x_preamble, 3))) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Frame preamble+start code mismatch");
    pnd->iLastError = ECOMIO;
    return -1;
  }

  if ((0x01 == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Error frame
    uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout);
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Application level error detected");
    pnd->iLastError = EFRAISERRFRAME;
    return -1;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout);
    if (pnd->iLastError) return -1;

    // (abtRxBuf[0] << 8) + abtRxBuf[1] (LEN) include TFI + (CC+1)
    len = (abtRxBuf[0] << 8) + abtRxBuf[1] - 2;
    if (((abtRxBuf[0] + abtRxBuf[1] + abtRxBuf[2]) % 256) != 0) {
      // TODO: Retry
      log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->iLastError = ECOMIO;
      return -1;
    }
  } else {
    // Normal frame
    if (256 != (abtRxBuf[3] + abtRxBuf[4])) {
      // TODO: Retry
      log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->iLastError = ECOMIO;
      return -1;
    }

    // abtRxBuf[3] (LEN) include TFI + (CC+1)
    len = abtRxBuf[3] - 2;
  }

  if (len > szDataLen) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %zu, len: %zu)", szDataLen, len);
    pnd->iLastError = ECOMIO;
    return -1;
  }

  // TFI + PD0 (CC+1)
  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->iLastError != 0) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return -1;
  }

  if (abtRxBuf[0] != 0xD5) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->iLastError = ECOMIO;
    return -1;
  }

  if (abtRxBuf[1] != CHIP_DATA (pnd)->ui8LastCommand + 1) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->iLastError = ECOMIO;
    return -1;
  }

  if (len) {
    pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, pbtData, len, 0, timeout);
    if (pnd->iLastError != 0) {
      log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      return -1;
    }
  }

  pnd->iLastError = uart_receive (DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->iLastError != 0) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return -1;
  }

  byte_t btDCS = (256 - 0xD5);
  btDCS -= CHIP_DATA (pnd)->ui8LastCommand + 1;
  for (size_t szPos = 0; szPos < len; szPos++) {
    btDCS -= pbtData[szPos];
  }

  if (btDCS != abtRxBuf[0]) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Data checksum mismatch");
    pnd->iLastError = ECOMIO;
    return -1;
  }

  if (0x00 != abtRxBuf[1]) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Frame postamble mismatch");
    pnd->iLastError = ECOMIO;
    return -1;
  }
  // The PN53x command is done and we successfully received the reply
  return len;
}

int
pn532_uart_ack (nfc_device_t * pnd)
{
  if (POWERDOWN == CHIP_DATA(pnd)->power_mode) {
    if (-1 == pn532_uart_wakeup(pnd)) {
      return -1;
    }
  }
  return (0 == uart_send (DRIVER_DATA(pnd)->port, pn53x_ack_frame, sizeof (pn53x_ack_frame),  NULL)) ? 0 : -1;
}

bool
pn532_uart_abort_command (nfc_device_t * pnd)
{
  if (pnd) {
#ifndef WIN32
    close (DRIVER_DATA (pnd)->iAbortFds[0]);
    pipe (DRIVER_DATA (pnd)->iAbortFds);
#else
    DRIVER_DATA (pnd)->abort_flag = true;
#endif
  }
  return true;
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
  .initiator_poll_target            = pn53x_initiator_poll_target,
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

  .abort_command  = pn532_uart_abort_command,
  .idle  = pn53x_idle,
};

