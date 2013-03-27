/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Additional contributors of this file:
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

#include "pn532_uart.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "uart.h"

#define PN532_UART_DEFAULT_SPEED 115200
#define PN532_UART_DRIVER_NAME "pn532_uart"

#define LOG_CATEGORY "libnfc.driver.pn532_uart"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

// Internal data structs
const struct pn53x_io pn532_uart_io;
struct pn532_uart_data {
  serial_port port;
#ifndef WIN32
  int     iAbortFds[2];
#else
  volatile bool abort_flag;
#endif
};

// Prototypes
int     pn532_uart_ack(nfc_device *pnd);
int     pn532_uart_wakeup(nfc_device *pnd);

#define DRIVER_DATA(pnd) ((struct pn532_uart_data*)(pnd->driver_data))

static size_t
pn532_uart_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;
  serial_port sp;
  char **acPorts = uart_list_ports();
  const char *acPort;
  int     iDevice = 0;

  while ((acPort = acPorts[iDevice++])) {
    sp = uart_open(acPort);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Trying to find PN532 device on serial port: %s at %d bauds.", acPort, PN532_UART_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      // We need to flush input to be sure first reply does not comes from older byte transceive
      uart_flush_input(sp);
      // Serial port claimed but we need to check if a PN532_UART is opened.
      uart_set_speed(sp, PN532_UART_DEFAULT_SPEED);

      nfc_connstring connstring;
      snprintf(connstring, sizeof(nfc_connstring), "%s:%s:%"PRIu32, PN532_UART_DRIVER_NAME, acPort, PN532_UART_DEFAULT_SPEED);
      nfc_device *pnd = nfc_device_new(context, connstring);
      if (!pnd) {
        perror("malloc");
        uart_close(sp);
        return 0;
      }
      pnd->driver = &pn532_uart_driver;
      pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
      if (!pnd->driver_data) {
        perror("malloc");
        uart_close(sp);
        nfc_device_free(pnd);
        return 0;
      }
      DRIVER_DATA(pnd)->port = sp;

      // Alloc and init chip's data
      if (pn53x_data_new(pnd, &pn532_uart_io) == NULL) {
        perror("malloc");
        uart_close(DRIVER_DATA(pnd)->port);
        nfc_device_free(pnd);
        return 0;
      }
      // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
      CHIP_DATA(pnd)->type = PN532;
      // This device starts in LowVBat power mode
      CHIP_DATA(pnd)->power_mode = LOWVBAT;

#ifndef WIN32
      // pipe-based abort mecanism
      if (pipe(DRIVER_DATA(pnd)->iAbortFds) < 0) {
        uart_close(DRIVER_DATA(pnd)->port);
        pn53x_data_free(pnd);
        nfc_device_free(pnd);
        return 0;
      }
#else
      DRIVER_DATA(pnd)->abort_flag = false;
#endif

      // Check communication using "Diagnose" command, with "Communication test" (0x00)
      int res = pn53x_check_communication(pnd);
      uart_close(DRIVER_DATA(pnd)->port);
      pn53x_data_free(pnd);
      nfc_device_free(pnd);
      if (res < 0) {
        continue;
      }

      memcpy(connstrings[device_found], connstring, sizeof(nfc_connstring));
      device_found++;

      // Test if we reach the maximum "wanted" devices
      if (device_found >= connstrings_len)
        break;
    }
  }
  iDevice = 0;
  while ((acPort = acPorts[iDevice++])) {
    free((void *)acPort);
  }
  free(acPorts);
  return device_found;
}

struct pn532_uart_descriptor {
  char *port;
  uint32_t speed;
};

static void
pn532_uart_close(nfc_device *pnd)
{
  pn53x_idle(pnd);

  // Release UART port
  uart_close(DRIVER_DATA(pnd)->port);

#ifndef WIN32
  // Release file descriptors used for abort mecanism
  close(DRIVER_DATA(pnd)->iAbortFds[0]);
  close(DRIVER_DATA(pnd)->iAbortFds[1]);
#endif

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

static nfc_device *
pn532_uart_open(const nfc_context *context, const nfc_connstring connstring)
{
  struct pn532_uart_descriptor ndd;
  char *speed_s;
  int connstring_decode_level = connstring_decode(connstring, PN532_UART_DRIVER_NAME, NULL, &ndd.port, &speed_s);
  if (connstring_decode_level == 3) {
    ndd.speed = 0;
    if (sscanf(speed_s, "%10"PRIu32, &ndd.speed) != 1) {
      // speed_s is not a number
      free(ndd.port);
      free(speed_s);
      return NULL;
    }
    free(speed_s);
  }
  if (connstring_decode_level < 2) {
    return NULL;
  }
  if (connstring_decode_level < 3) {
    ndd.speed = PN532_UART_DEFAULT_SPEED;
  }
  serial_port sp;
  nfc_device *pnd = NULL;

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Attempt to open: %s at %d bauds.", ndd.port, ndd.speed);
  sp = uart_open(ndd.port);

  if (sp == INVALID_SERIAL_PORT)
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Invalid serial port: %s", ndd.port);
  if (sp == CLAIMED_SERIAL_PORT)
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Serial port already claimed: %s", ndd.port);
  if ((sp == CLAIMED_SERIAL_PORT) || (sp == INVALID_SERIAL_PORT)) {
    free(ndd.port);
    return NULL;
  }
  // We need to flush input to be sure first reply does not comes from older byte transceive
  uart_flush_input(sp);
  uart_set_speed(sp, ndd.speed);

  // We have a connection
  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    free(ndd.port);
    uart_close(sp);
    return NULL;
  }
  snprintf(pnd->name, sizeof(pnd->name), "%s:%s", PN532_UART_DRIVER_NAME, ndd.port);
  free(ndd.port);

  pnd->driver_data = malloc(sizeof(struct pn532_uart_data));
  if (!pnd->driver_data) {
    perror("malloc");
    uart_close(sp);
    nfc_device_free(pnd);
    return NULL;
  }
  DRIVER_DATA(pnd)->port = sp;

  // Alloc and init chip's data
  if (pn53x_data_new(pnd, &pn532_uart_io) == NULL) {
    perror("malloc");
    uart_close(DRIVER_DATA(pnd)->port);
    nfc_device_free(pnd);
    return NULL;
  }
  // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
  CHIP_DATA(pnd)->type = PN532;
  // This device starts in LowVBat mode
  CHIP_DATA(pnd)->power_mode = LOWVBAT;

  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 48;
  pnd->driver = &pn532_uart_driver;

#ifndef WIN32
  // pipe-based abort mecanism
  if (pipe(DRIVER_DATA(pnd)->iAbortFds) < 0) {
    uart_close(DRIVER_DATA(pnd)->port);
    pn53x_data_free(pnd);
    nfc_device_free(pnd);
    return NULL;
  }
#else
  DRIVER_DATA(pnd)->abort_flag = false;
#endif

  // Check communication using "Diagnose" command, with "Communication test" (0x00)
  if (pn53x_check_communication(pnd) < 0) {
    nfc_perror(pnd, "pn53x_check_communication");
    pn532_uart_close(pnd);
    return NULL;
  }

  pn53x_init(pnd);
  return pnd;
}

int
pn532_uart_wakeup(nfc_device *pnd)
{
  /* High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for PN532 being wakeup. */
  const uint8_t pn532_wakeup_preamble[] = { 0x55, 0x55, 0x00, 0x00, 0x00 };
  int res = uart_send(DRIVER_DATA(pnd)->port, pn532_wakeup_preamble, sizeof(pn532_wakeup_preamble), 0);
  CHIP_DATA(pnd)->power_mode = NORMAL; // PN532 should now be awake
  return res;
}

#define PN532_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
static int
pn532_uart_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout)
{
  int res = 0;
  // Before sending anything, we need to discard from any junk bytes
  uart_flush_input(DRIVER_DATA(pnd)->port);

  switch (CHIP_DATA(pnd)->power_mode) {
    case LOWVBAT: {
      /** PN532C106 wakeup. */
      if ((res = pn532_uart_wakeup(pnd)) < 0) {
        return res;
      }
      // According to PN532 application note, C106 appendix: to go out Low Vbat mode and enter in normal mode we need to send a SAMConfiguration command
      if ((res = pn532_SAMConfiguration(pnd, PSM_NORMAL, 1000)) < 0) {
        return res;
      }
    }
    break;
    case POWERDOWN: {
      if ((res = pn532_uart_wakeup(pnd)) < 0) {
        return res;
      }
    }
    break;
    case NORMAL:
      // Nothing to do :)
      break;
  };

  uint8_t  abtFrame[PN532_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  size_t szFrame = 0;

  if ((res = pn53x_build_frame(abtFrame, &szFrame, pbtData, szData)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }

  res = uart_send(DRIVER_DATA(pnd)->port, abtFrame, szFrame, timeout);
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->last_error = res;
    return pnd->last_error;
  }

  uint8_t abtRxBuf[6];
  res = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 6, 0, timeout);
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Unable to read ACK");
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (pn53x_check_ack_frame(pnd, abtRxBuf, sizeof(abtRxBuf)) == 0) {
    // The PN53x is running the sent command
  } else {
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

static int
pn532_uart_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout)
{
  uint8_t  abtRxBuf[5];
  size_t len;
  void *abort_p = NULL;

#ifndef WIN32
  abort_p = &(DRIVER_DATA(pnd)->iAbortFds[1]);
#else
  abort_p = (void *) & (DRIVER_DATA(pnd)->abort_flag);
#endif

  pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 5, abort_p, timeout);

  if (abort_p && (NFC_EOPABORTED == pnd->last_error)) {
    return pn532_uart_ack(pnd);
  }

  if (pnd->last_error < 0) {
    goto error;
  }

  const uint8_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };
  if (0 != (memcmp(abtRxBuf, pn53x_preamble, 3))) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame preamble+start code mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if ((0x01 == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Error frame
    uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Application level error detected");
    pnd->last_error = NFC_EIO;
    goto error;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout);
    if (pnd->last_error != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      goto error;
    }
    // (abtRxBuf[0] << 8) + abtRxBuf[1] (LEN) include TFI + (CC+1)
    len = (abtRxBuf[0] << 8) + abtRxBuf[1] - 2;
    if (((abtRxBuf[0] + abtRxBuf[1] + abtRxBuf[2]) % 256) != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }
  } else {
    // Normal frame
    if (256 != (abtRxBuf[3] + abtRxBuf[4])) {
      // TODO: Retry
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }

    // abtRxBuf[3] (LEN) include TFI + (CC+1)
    len = abtRxBuf[3] - 2;
  }

  if (len > szDataLen) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %" PRIuPTR ", len: %" PRIuPTR ")", szDataLen, len);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  // TFI + PD0 (CC+1)
  pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    goto error;
  }

  if (abtRxBuf[0] != 0xD5) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (abtRxBuf[1] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (len) {
    pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, pbtData, len, 0, timeout);
    if (pnd->last_error != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      goto error;
    }
  }

  pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    goto error;
  }

  uint8_t btDCS = (256 - 0xD5);
  btDCS -= CHIP_DATA(pnd)->last_command + 1;
  for (size_t szPos = 0; szPos < len; szPos++) {
    btDCS -= pbtData[szPos];
  }

  if (btDCS != abtRxBuf[0]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Data checksum mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (0x00 != abtRxBuf[1]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame postamble mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }
  // The PN53x command is done and we successfully received the reply
  return len;
error:
  uart_flush_input(DRIVER_DATA(pnd)->port);
  return pnd->last_error;
}

int
pn532_uart_ack(nfc_device *pnd)
{
  if (POWERDOWN == CHIP_DATA(pnd)->power_mode) {
    int res = 0;
    if ((res = pn532_uart_wakeup(pnd)) < 0) {
      return res;
    }
  }
  return (uart_send(DRIVER_DATA(pnd)->port, pn53x_ack_frame, sizeof(pn53x_ack_frame),  0));
}

static int
pn532_uart_abort_command(nfc_device *pnd)
{
  if (pnd) {
#ifndef WIN32
    close(DRIVER_DATA(pnd)->iAbortFds[0]);
    if (pipe(DRIVER_DATA(pnd)->iAbortFds) < 0) {
      return NFC_ESOFT;
    }
#else
    DRIVER_DATA(pnd)->abort_flag = true;
#endif
  }
  return NFC_SUCCESS;
}

const struct pn53x_io pn532_uart_io = {
  .send       = pn532_uart_send,
  .receive    = pn532_uart_receive,
};

const struct nfc_driver pn532_uart_driver = {
  .name                             = PN532_UART_DRIVER_NAME,
  .scan_type                        = INTRUSIVE,
  .scan                             = pn532_uart_scan,
  .open                             = pn532_uart_open,
  .close                            = pn532_uart_close,
  .strerror                         = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_init_secure_element    = pn532_initiator_init_secure_element,
  .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
  .initiator_poll_target            = pn53x_initiator_poll_target,
  .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
  .initiator_deselect_target        = pn53x_initiator_deselect_target,
  .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
  .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
  .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
  .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,
  .initiator_target_is_present      = pn53x_initiator_target_is_present,

  .target_init           = pn53x_target_init,
  .target_send_bytes     = pn53x_target_send_bytes,
  .target_receive_bytes  = pn53x_target_receive_bytes,
  .target_send_bits      = pn53x_target_send_bits,
  .target_receive_bits   = pn53x_target_receive_bits,

  .device_set_property_bool     = pn53x_set_property_bool,
  .device_set_property_int      = pn53x_set_property_int,
  .get_supported_modulation     = pn53x_get_supported_modulation,
  .get_supported_baud_rate      = pn53x_get_supported_baud_rate,
  .device_get_information_about = pn53x_get_information_about,

  .abort_command  = pn532_uart_abort_command,
  .idle           = pn53x_idle,
  .powerdown      = pn53x_PowerDown,
};

