/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
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
#include <inttypes.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
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

#define ARYGON_DEFAULT_SPEED 9600
#define ARYGON_DRIVER_NAME "arygon"

#define LOG_CATEGORY "libnfc.driver.arygon"
#define LOG_GROUP     NFC_LOG_GROUP_DRIVER

#define DRIVER_DATA(pnd) ((struct arygon_data*)(pnd->driver_data))

// Internal data structs
const struct pn53x_io arygon_tama_io;

struct arygon_data {
  serial_port port;
#ifndef WIN32
  int     iAbortFds[2];
#else
  volatile bool abort_flag;
#endif
};

// ARYGON frames
static const uint8_t arygon_error_none[] = "FF000000\x0d\x0a";
static const uint8_t arygon_error_incomplete_command[] = "FF0C0000\x0d\x0a";
static const uint8_t arygon_error_unknown_mode[] = "FF060000\x0d\x0a";

// Prototypes
int     arygon_reset_tama(nfc_device *pnd);
void    arygon_firmware(nfc_device *pnd, char *str);

static size_t
arygon_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;
  serial_port sp;
  char **acPorts = uart_list_ports();
  const char *acPort;
  int     iDevice = 0;

  while ((acPort = acPorts[iDevice++])) {
    sp = uart_open(acPort);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Trying to find ARYGON device on serial port: %s at %d bauds.", acPort, ARYGON_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      // We need to flush input to be sure first reply does not comes from older byte transceive
      uart_flush_input(sp, true);
      uart_set_speed(sp, ARYGON_DEFAULT_SPEED);

      nfc_connstring connstring;
      snprintf(connstring, sizeof(nfc_connstring), "%s:%s:%"PRIu32, ARYGON_DRIVER_NAME, acPort, ARYGON_DEFAULT_SPEED);
      nfc_device *pnd = nfc_device_new(context, connstring);
      if (!pnd) {
        perror("malloc");
        uart_close(sp);
        iDevice = 0;
        while ((acPort = acPorts[iDevice++])) {
          free((void *)acPort);
        }
        free(acPorts);
        return 0;
      }

      pnd->driver = &arygon_driver;
      pnd->driver_data = malloc(sizeof(struct arygon_data));
      if (!pnd->driver_data) {
        perror("malloc");
        uart_close(sp);
        nfc_device_free(pnd);
        iDevice = 0;
        while ((acPort = acPorts[iDevice++])) {
          free((void *)acPort);
        }
        free(acPorts);
        return 0;
      }
      DRIVER_DATA(pnd)->port = sp;

      // Alloc and init chip's data
      if (pn53x_data_new(pnd, &arygon_tama_io) == NULL) {
        perror("malloc");
        uart_close(DRIVER_DATA(pnd)->port);
        nfc_device_free(pnd);
        iDevice = 0;
        while ((acPort = acPorts[iDevice++])) {
          free((void *)acPort);
        }
        free(acPorts);
        return 0;
      }

#ifndef WIN32
      // pipe-based abort mecanism
      if (pipe(DRIVER_DATA(pnd)->iAbortFds) < 0) {
        uart_close(DRIVER_DATA(pnd)->port);
        pn53x_data_free(pnd);
        nfc_device_free(pnd);
        iDevice = 0;
        while ((acPort = acPorts[iDevice++])) {
          free((void *)acPort);
        }
        free(acPorts);
        return 0;
      }
#else
      DRIVER_DATA(pnd)->abort_flag = false;
#endif

      int res = arygon_reset_tama(pnd);
      uart_close(DRIVER_DATA(pnd)->port);
      pn53x_data_free(pnd);
      nfc_device_free(pnd);
      if (res < 0) {
        continue;
      }

      // ARYGON reader is found
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

struct arygon_descriptor {
  char *port;
  uint32_t speed;
};

static void
arygon_close_step2(nfc_device *pnd)
{
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

static void
arygon_close(nfc_device *pnd)
{
  pn53x_idle(pnd);
  arygon_close_step2(pnd);
}

static nfc_device *
arygon_open(const nfc_context *context, const nfc_connstring connstring)
{
  struct arygon_descriptor ndd;
  char *speed_s;
  int connstring_decode_level = connstring_decode(connstring, ARYGON_DRIVER_NAME, NULL, &ndd.port, &speed_s);
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
    ndd.speed = ARYGON_DEFAULT_SPEED;
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
  uart_flush_input(sp, true);
  uart_set_speed(sp, ndd.speed);

  // We have a connection
  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    free(ndd.port);
    uart_close(sp);
    return NULL;
  }
  snprintf(pnd->name, sizeof(pnd->name), "%s:%s", ARYGON_DRIVER_NAME, ndd.port);
  free(ndd.port);

  pnd->driver_data = malloc(sizeof(struct arygon_data));
  if (!pnd->driver_data) {
    perror("malloc");
    uart_close(sp);
    nfc_device_free(pnd);
    return NULL;
  }
  DRIVER_DATA(pnd)->port = sp;

  // Alloc and init chip's data
  if (pn53x_data_new(pnd, &arygon_tama_io) == NULL) {
    perror("malloc");
    uart_close(DRIVER_DATA(pnd)->port);
    nfc_device_free(pnd);
    return NULL;
  }

  // The PN53x chip opened to ARYGON MCU doesn't seems to be in LowVBat mode
  CHIP_DATA(pnd)->power_mode = NORMAL;

  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 46;
  pnd->driver = &arygon_driver;

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

  // Check communication using "Reset TAMA" command
  if (arygon_reset_tama(pnd) < 0) {
    arygon_close_step2(pnd);
    return NULL;
  }

  char arygon_firmware_version[10];
  arygon_firmware(pnd, arygon_firmware_version);
  char   *pcName;
  pcName = strdup(pnd->name);
  snprintf(pnd->name, sizeof(pnd->name), "%s %s", pcName, arygon_firmware_version);
  free(pcName);

  pn53x_init(pnd);
  return pnd;
}

#define ARYGON_TX_BUFFER_LEN (PN53x_NORMAL_FRAME__DATA_MAX_LEN + PN53x_NORMAL_FRAME__OVERHEAD + 1)
#define ARYGON_RX_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
static int
arygon_tama_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout)
{
  int res = 0;
  // Before sending anything, we need to discard from any junk bytes
  uart_flush_input(DRIVER_DATA(pnd)->port, false);

  uint8_t abtFrame[ARYGON_TX_BUFFER_LEN] = { DEV_ARYGON_PROTOCOL_TAMA, 0x00, 0x00, 0xff };     // Every packet must start with "0x32 0x00 0x00 0xff"

  size_t szFrame = 0;
  if (szData > PN53x_NORMAL_FRAME__DATA_MAX_LEN) {
    // ARYGON Reader with PN532 equipped does not support extended frame (bug in ARYGON firmware?)
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "ARYGON device does not support more than %d bytes as payload (requested: %" PRIdPTR ")", PN53x_NORMAL_FRAME__DATA_MAX_LEN, szData);
    pnd->last_error = NFC_EDEVNOTSUPP;
    return pnd->last_error;
  }

  if ((res = pn53x_build_frame(abtFrame + 1, &szFrame, pbtData, szData)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }

  if ((res = uart_send(DRIVER_DATA(pnd)->port, abtFrame, szFrame + 1, timeout)) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->last_error = res;
    return pnd->last_error;
  }

  uint8_t abtRxBuf[PN53x_ACK_FRAME__LEN];
  if ((res = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, sizeof(abtRxBuf), 0, timeout)) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to read ACK");
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (pn53x_check_ack_frame(pnd, abtRxBuf, sizeof(abtRxBuf)) == 0) {
    // The PN53x is running the sent command
  } else if (0 == memcmp(arygon_error_unknown_mode, abtRxBuf, sizeof(abtRxBuf))) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Bad frame format.");
    // We have already read 6 bytes and arygon_error_unknown_mode is 10 bytes long
    // so we have to read 4 remaining bytes to be synchronized at the next receiving pass.
    pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 4, 0, timeout);
    return pnd->last_error;
  } else {
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

static int
arygon_abort(nfc_device *pnd)
{
  // Send a valid TAMA packet to wakup the PN53x (we will not have an answer, according to Arygon manual)
  uint8_t dummy[] = { 0x32, 0x00, 0x00, 0xff, 0x09, 0xf7, 0xd4, 0x00, 0x00, 0x6c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0xbe, 0x00 };

  uart_send(DRIVER_DATA(pnd)->port, dummy, sizeof(dummy), 0);

  // Using Arygon device we can't send ACK frame to abort the running command
  return pn53x_check_communication(pnd);
}

static int
arygon_tama_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout)
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
    arygon_abort(pnd);

    /* last_error got reset by arygon_abort() */
    pnd->last_error = NFC_EOPABORTED;
    return pnd->last_error;
  }

  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return pnd->last_error;
  }

  const uint8_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };
  if (0 != (memcmp(abtRxBuf, pn53x_preamble, 3))) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame preamble+start code mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  if ((0x01 == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Error frame
    uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Application level error detected");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    // ARYGON devices does not support extended frame sending
    abort();
  } else {
    // Normal frame
    if (256 != (abtRxBuf[3] + abtRxBuf[4])) {
      // TODO: Retry
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }

    // abtRxBuf[3] (LEN) include TFI + (CC+1)
    len = abtRxBuf[3] - 2;
  }

  if (len > szDataLen) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %" PRIuPTR ", len: %" PRIuPTR ")", szDataLen, len);
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  // TFI + PD0 (CC+1)
  pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return pnd->last_error;
  }

  if (abtRxBuf[0] != 0xD5) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  if (abtRxBuf[1] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  if (len) {
    pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, pbtData, len, 0, timeout);
    if (pnd->last_error != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      return pnd->last_error;
    }
  }

  pnd->last_error = uart_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return pnd->last_error;
  }

  uint8_t btDCS = (256 - 0xD5);
  btDCS -= CHIP_DATA(pnd)->last_command + 1;
  for (size_t szPos = 0; szPos < len; szPos++) {
    btDCS -= pbtData[szPos];
  }

  if (btDCS != abtRxBuf[0]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Data checksum mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  if (0x00 != abtRxBuf[1]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame postamble mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  // The PN53x command is done and we successfully received the reply
  return len;
}

void
arygon_firmware(nfc_device *pnd, char *str)
{
  const uint8_t arygon_firmware_version_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'v' };
  uint8_t abtRx[16];
  size_t szRx = sizeof(abtRx);


  int res = uart_send(DRIVER_DATA(pnd)->port, arygon_firmware_version_cmd, sizeof(arygon_firmware_version_cmd), 0);
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Unable to send ARYGON firmware command.");
    return;
  }
  res = uart_receive(DRIVER_DATA(pnd)->port, abtRx, szRx, 0, 0);
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Unable to retrieve ARYGON firmware version.");
    return;
  }

  if (0 == memcmp(abtRx, arygon_error_none, 6)) {
    uint8_t *p = abtRx + 6;
    unsigned int szData;
    sscanf((const char *)p, "%02x%9s", &szData, p);
    if (szData > 9)
      szData = 9;
    memcpy(str, p, szData);
    *(str + szData) = '\0';
  }
}

int
arygon_reset_tama(nfc_device *pnd)
{
  const uint8_t arygon_reset_tama_cmd[] = { DEV_ARYGON_PROTOCOL_ARYGON_ASCII, 'a', 'r' };
  uint8_t abtRx[10]; // Attempted response is 10 bytes long
  size_t szRx = sizeof(abtRx);
  int res;

  uart_send(DRIVER_DATA(pnd)->port, arygon_reset_tama_cmd, sizeof(arygon_reset_tama_cmd), 500);

  // Two reply are possible from ARYGON device: arygon_error_none (ie. in case the byte is well-sent)
  // or arygon_error_unknown_mode (ie. in case of the first byte was bad-transmitted)
  res = uart_receive(DRIVER_DATA(pnd)->port, abtRx, szRx, 0, 1000);
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "No reply to 'reset TAMA' command.");
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (0 != memcmp(abtRx, arygon_error_none, sizeof(arygon_error_none) - 1)) {
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  return NFC_SUCCESS;
}

static int
arygon_abort_command(nfc_device *pnd)
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


const struct pn53x_io arygon_tama_io = {
  .send       = arygon_tama_send,
  .receive    = arygon_tama_receive,
};

const struct nfc_driver arygon_driver = {
  .name                             = ARYGON_DRIVER_NAME,
  .scan_type                        = INTRUSIVE,
  .scan                             = arygon_scan,
  .open                             = arygon_open,
  .close                            = arygon_close,
  .strerror                         = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_init_secure_element    = NULL, // No secure-element support
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

  .abort_command  = arygon_abort_command,
  .idle           = pn53x_idle,
  /* Even if PN532, PowerDown is not recommended on those devices */
  .powerdown      = NULL,
};

