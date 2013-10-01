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
 * Copyright (C) 2011      Anugrah Redja Kusuma
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
 * @file acr122s.c
 * @brief Driver for ACS ACR122S devices
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "acr122s.h"

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

#define ACR122S_DEFAULT_SPEED 9600
#define ACR122S_DRIVER_NAME "ACR122S"

#define LOG_CATEGORY "libnfc.driver.acr122s"
#define LOG_GROUP     NFC_LOG_GROUP_DRIVER

// Internal data structs
struct acr122s_data {
  serial_port port;
  uint8_t seq;
#ifndef WIN32
  int abort_fds[2];
#else
  volatile bool abort_flag;
#endif
};

const struct pn53x_io acr122s_io;

#define STX 2
#define ETX 3

#define APDU_SIZE(p) ((uint32_t) (p[2] | p[3] << 8 | p[4] << 16 | p[5] << 24))
#define FRAME_OVERHEAD 13
#define FRAME_SIZE(p) (APDU_SIZE(p) + FRAME_OVERHEAD)
#define MAX_FRAME_SIZE (FRAME_OVERHEAD + 5 + 255)

enum {
  ICC_POWER_ON_REQ_MSG  = 0x62,
  ICC_POWER_OFF_REQ_MSG = 0x63,
  XFR_BLOCK_REQ_MSG     = 0x6F,

  ICC_POWER_ON_RES_MSG  = 0x80,
  ICC_POWER_OFF_RES_MSG = 0x81,
  XFR_BLOCK_RES_MSG     = 0x80,
};

enum {
  POWER_AUTO  = 0,
  POWER_5_0_V = 1,
  POWER_3_0_V = 2,
  POWER_1_8_V = 3,
};

#pragma pack(push, 1)

struct icc_power_on_req {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t power_select;
  uint8_t rfu[2];
};

struct icc_power_on_res {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t status;
  uint8_t error;
  uint8_t chain_parameter;
};

struct icc_power_off_req {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t rfu[3];
};

struct icc_power_off_res {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t status;
  uint8_t error;
  uint8_t clock_status;
};

struct xfr_block_req {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t bwi;
  uint8_t rfu[2];
};

struct xfr_block_res {
  uint8_t message_type;
  uint32_t length;
  uint8_t slot;
  uint8_t seq;
  uint8_t status;
  uint8_t error;
  uint8_t chain_parameter;
};

struct apdu_header {
  uint8_t class;
  uint8_t ins;
  uint8_t p1;
  uint8_t p2;
  uint8_t length;
};

#pragma pack(pop)

#define DRIVER_DATA(pnd) ((struct acr122s_data *) (pnd->driver_data))

/**
 * Fix a command frame with a valid prefix, checksum, and suffix.
 *
 * @param frame is command frame to fix
 * @note command frame length (uint32_t at offset 2) should be valid
 */
static void
acr122s_fix_frame(uint8_t *frame)
{
  size_t frame_size = FRAME_SIZE(frame);
  frame[0] = STX;
  frame[frame_size - 1] = ETX;

  uint8_t *csum = frame + frame_size - 2;
  *csum = 0;
  for (uint8_t *p = frame + 1; p < csum; p++)
    *csum ^= *p;
}

/**
 * Send a command frame to ACR122S and check its ACK status.
 *
 * @param: pnd is target nfc device
 * @param: cmd is command frame to send
 * @param: timeout
 * @return 0 if success
 */
static int
acr122s_send_frame(nfc_device *pnd, uint8_t *frame, int timeout)
{
  size_t frame_size = FRAME_SIZE(frame);
  uint8_t ack[4];
  uint8_t positive_ack[4] = { STX, 0, 0, ETX };
  serial_port port = DRIVER_DATA(pnd)->port;
  int ret;
  void *abort_p;

#ifndef WIN32
  abort_p = &(DRIVER_DATA(pnd)->abort_fds[1]);
#else
  abort_p = &(DRIVER_DATA(pnd)->abort_flag);
#endif

  if ((ret = uart_send(port, frame, frame_size, timeout)) < 0)
    return ret;

  if ((ret = uart_receive(port, ack, 4, abort_p, timeout)) < 0)
    return ret;

  if (memcmp(ack, positive_ack, 4) != 0) {
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  struct xfr_block_req *req = (struct xfr_block_req *) &frame[1];
  DRIVER_DATA(pnd)->seq = req->seq + 1;

  return 0;
}

/**
 * Receive response frame after a successfull acr122s_send_command().
 *
 * @param: pnd is target nfc device
 * @param: frame is buffer where received response frame will be stored
 * @param: frame_size is frame size
 * @param: abort_p
 * @param: timeout
 * @note returned frame size can be fetched using FRAME_SIZE macro
 *
 * @return 0 if success
 */
static int
acr122s_recv_frame(nfc_device *pnd, uint8_t *frame, size_t frame_size, void *abort_p, int timeout)
{
  if (frame_size < 13) {
    pnd->last_error = NFC_EINVARG;
    return pnd->last_error;
  }
  int ret;
  serial_port port = DRIVER_DATA(pnd)->port;

  if ((ret = uart_receive(port, frame, 11, abort_p, timeout)) != 0)
    return ret;

  // Is buffer sufficient to store response?
  if (frame_size < FRAME_SIZE(frame)) {
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  size_t remaining = FRAME_SIZE(frame) - 11;
  if ((ret = uart_receive(port, frame + 11, remaining, abort_p, timeout)) != 0)
    return ret;

  struct xfr_block_res *res = (struct xfr_block_res *) &frame[1];
  if ((uint8_t)(res->seq + 1) != DRIVER_DATA(pnd)->seq) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Invalid response sequence number.");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  return 0;
}

#define APDU_OVERHEAD (FRAME_OVERHEAD + 5)

/**
 * Convert host uint32 to litle endian uint32
 */
static uint32_t
le32(uint32_t val)
{
  uint32_t res;
  uint8_t *p = (uint8_t *) &res;
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
  return res;
}

/**
 * Build an ACR122S command frame from a PN532 command.
 *
 * @param pnd is device for which the command frame will be generated
 * @param frame is where the resulting command frame will be generated
 * @param frame_size is the passed command frame size
 * @param p1
 * @param p2
 * @param data is PN532 APDU data without the direction prefix (0xD4)
 * @param data_size is APDU data size
 * @param should_prefix 1 if prefix 0xD4 should be inserted before APDU data, 0 if not
 *
 * @return true if frame built successfully
 */
static bool
acr122s_build_frame(nfc_device *pnd,
                    uint8_t *frame, size_t frame_size, uint8_t p1, uint8_t p2,
                    const uint8_t *data, size_t data_size, int should_prefix)
{
  if (frame_size < data_size + APDU_OVERHEAD + should_prefix)
    return false;
  if (data_size + should_prefix > 255)
    return false;
  if (data == NULL)
    return false;

  struct xfr_block_req *req = (struct xfr_block_req *) &frame[1];
  req->message_type = XFR_BLOCK_REQ_MSG;
  req->length = le32(5 + data_size + should_prefix);
  req->slot = 0;
  req->seq = DRIVER_DATA(pnd)->seq;
  req->bwi = 0;
  req->rfu[0] = 0;
  req->rfu[1] = 0;

  struct apdu_header *header = (struct apdu_header *) &frame[11];
  header->class = 0xff;
  header->ins = 0;
  header->p1 = p1;
  header->p2 = p2;
  header->length = data_size + should_prefix;

  uint8_t *buf = (uint8_t *) &frame[16];
  if (should_prefix)
    *buf++ = 0xD4;
  memcpy(buf, data, data_size);
  acr122s_fix_frame(frame);

  return true;
}

static int
acr122s_activate_sam(nfc_device *pnd)
{
  uint8_t cmd[13];
  memset(cmd, 0, sizeof(cmd));
  cmd[1] = ICC_POWER_ON_REQ_MSG;
  acr122s_fix_frame(cmd);

  uint8_t resp[MAX_FRAME_SIZE];
  int ret;

  if ((ret = acr122s_send_frame(pnd, cmd, 0)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(pnd, resp, MAX_FRAME_SIZE, 0, 0)) != 0)
    return ret;

  CHIP_DATA(pnd)->power_mode = NORMAL;

  return 0;
}

static int
acr122s_deactivate_sam(nfc_device *pnd)
{
  uint8_t cmd[13];
  memset(cmd, 0, sizeof(cmd));
  cmd[1] = ICC_POWER_OFF_REQ_MSG;
  acr122s_fix_frame(cmd);

  uint8_t resp[MAX_FRAME_SIZE];
  int ret;

  if ((ret = acr122s_send_frame(pnd, cmd, 0)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(pnd, resp, MAX_FRAME_SIZE, 0, 0)) != 0)
    return ret;

  CHIP_DATA(pnd)->power_mode = LOWVBAT;

  return 0;
}

static int
acr122s_get_firmware_version(nfc_device *pnd, char *version, size_t length)
{
  int ret;
  uint8_t cmd[MAX_FRAME_SIZE];

  if (! acr122s_build_frame(pnd, cmd, sizeof(cmd), 0x48, 0, NULL, 0, 0)) {
    return NFC_EINVARG;
  }

  if ((ret = acr122s_send_frame(pnd, cmd, 1000)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(pnd, cmd, sizeof(cmd), 0, 0)) != 0)
    return ret;

  size_t len = APDU_SIZE(cmd);
  if (len + 1 > length)
    len = length - 1;
  memcpy(version, cmd + 11, len);
  version[len] = 0;

  return 0;
}

struct acr122s_descriptor {
  char *port;
  uint32_t speed;
};

static size_t
acr122s_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;
  serial_port sp;
  char **acPorts = uart_list_ports();
  const char *acPort;
  int     iDevice = 0;

  while ((acPort = acPorts[iDevice++])) {
    sp = uart_open(acPort);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Trying to find ACR122S device on serial port: %s at %d bauds.", acPort, ACR122S_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      // We need to flush input to be sure first reply does not comes from older byte transceive
      uart_flush_input(sp, true);
      uart_set_speed(sp, ACR122S_DEFAULT_SPEED);

      nfc_connstring connstring;
      snprintf(connstring, sizeof(nfc_connstring), "%s:%s:%"PRIu32, ACR122S_DRIVER_NAME, acPort, ACR122S_DEFAULT_SPEED);
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

      pnd->driver = &acr122s_driver;
      pnd->driver_data = malloc(sizeof(struct acr122s_data));
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
      DRIVER_DATA(pnd)->seq = 0;

#ifndef WIN32
      if (pipe(DRIVER_DATA(pnd)->abort_fds) < 0) {
        uart_close(DRIVER_DATA(pnd)->port);
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

      if (pn53x_data_new(pnd, &acr122s_io) == NULL) {
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
      CHIP_DATA(pnd)->type = PN532;
      CHIP_DATA(pnd)->power_mode = NORMAL;

      char version[32];
      int ret = acr122s_get_firmware_version(pnd, version, sizeof(version));
      if (ret == 0 && strncmp("ACR122S", version, 7) != 0) {
        ret = -1;
      }

      uart_close(DRIVER_DATA(pnd)->port);
      pn53x_data_free(pnd);
      nfc_device_free(pnd);

      if (ret != 0)
        continue;

      // ACR122S reader is found
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

static void
acr122s_close(nfc_device *pnd)
{
  acr122s_deactivate_sam(pnd);
  pn53x_idle(pnd);

  uart_close(DRIVER_DATA(pnd)->port);

#ifndef WIN32
  // Release file descriptors used for abort mecanism
  close(DRIVER_DATA(pnd)->abort_fds[0]);
  close(DRIVER_DATA(pnd)->abort_fds[1]);
#endif

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

static nfc_device *
acr122s_open(const nfc_context *context, const nfc_connstring connstring)
{
  serial_port sp;
  nfc_device *pnd;
  struct acr122s_descriptor ndd;
  char *speed_s;
  int connstring_decode_level = connstring_decode(connstring, ACR122S_DRIVER_NAME, NULL, &ndd.port, &speed_s);
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
    ndd.speed = ACR122S_DEFAULT_SPEED;
  }

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
          "Attempt to connect to: %s at %d bauds.", ndd.port, ndd.speed);

  sp = uart_open(ndd.port);
  if (sp == INVALID_SERIAL_PORT) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR,
            "Invalid serial port: %s", ndd.port);
    free(ndd.port);
    return NULL;
  }
  if (sp == CLAIMED_SERIAL_PORT) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR,
            "Serial port already claimed: %s", ndd.port);
    free(ndd.port);
    return NULL;
  }

  uart_flush_input(sp, true);
  uart_set_speed(sp, ndd.speed);

  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    free(ndd.port);
    uart_close(sp);
    return NULL;
  }
  pnd->driver = &acr122s_driver;
  strcpy(pnd->name, ACR122S_DRIVER_NAME);
  free(ndd.port);

  pnd->driver_data = malloc(sizeof(struct acr122s_data));
  if (!pnd->driver_data) {
    perror("malloc");
    uart_close(sp);
    nfc_device_free(pnd);
    return NULL;
  }

  DRIVER_DATA(pnd)->port = sp;
  DRIVER_DATA(pnd)->seq = 0;

#ifndef WIN32
  if (pipe(DRIVER_DATA(pnd)->abort_fds) < 0) {
    uart_close(DRIVER_DATA(pnd)->port);
    nfc_device_free(pnd);
    return NULL;
  }
#else
  DRIVER_DATA(pnd)->abort_flag = false;
#endif

  if (pn53x_data_new(pnd, &acr122s_io) == NULL) {
    perror("malloc");
    uart_close(DRIVER_DATA(pnd)->port);
    nfc_device_free(pnd);
    return NULL;
  }
  CHIP_DATA(pnd)->type = PN532;

#if 1
  // Retrieve firmware version
  char version[DEVICE_NAME_LENGTH];
  if (acr122s_get_firmware_version(pnd, version, sizeof(version)) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Cannot get reader firmware.");
    acr122s_close(pnd);
    return NULL;
  }

  if (strncmp(version, "ACR122S", 7) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Invalid firmware version: %s",
            version);
    acr122s_close(pnd);
    return NULL;
  }

  snprintf(pnd->name, sizeof(pnd->name), "%s", version);

  // Activate SAM before operating
  if (acr122s_activate_sam(pnd) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Cannot activate SAM.");
    acr122s_close(pnd);
    return NULL;
  }
#endif

  if (pn53x_init(pnd) < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Failed initializing PN532 chip.");
    acr122s_close(pnd);
    return NULL;
  }

  return pnd;
}

static int
acr122s_send(nfc_device *pnd, const uint8_t *buf, const size_t buf_len, int timeout)
{
  uart_flush_input(DRIVER_DATA(pnd)->port, false);

  uint8_t cmd[MAX_FRAME_SIZE];
  if (! acr122s_build_frame(pnd, cmd, sizeof(cmd), 0, 0, buf, buf_len, 1)) {
    return NFC_EINVARG;
  }

  int ret;
  if ((ret = acr122s_send_frame(pnd, cmd, timeout)) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->last_error = ret;
    return pnd->last_error;
  }

  return NFC_SUCCESS;
}

static int
acr122s_receive(nfc_device *pnd, uint8_t *buf, size_t buf_len, int timeout)
{
  void *abort_p;

#ifndef WIN32
  abort_p = &(DRIVER_DATA(pnd)->abort_fds[1]);
#else
  abort_p = &(DRIVER_DATA(pnd)->abort_flag);
#endif

  uint8_t tmp[MAX_FRAME_SIZE];
  pnd->last_error = acr122s_recv_frame(pnd, tmp, sizeof(tmp), abort_p, timeout);

  if (abort_p && (NFC_EOPABORTED == pnd->last_error)) {
    pnd->last_error = NFC_EOPABORTED;
    return pnd->last_error;
  }

  if (pnd->last_error < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return -1;
  }

  size_t data_len = FRAME_SIZE(tmp) - 17;
  if (data_len > buf_len) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Receive buffer too small. (buf_len: %" PRIuPTR ", data_len: %" PRIuPTR ")", buf_len, data_len);
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  memcpy(buf, tmp + 13, data_len);
  return data_len;
}

static int
acr122s_abort_command(nfc_device *pnd)
{
  if (pnd) {
#ifndef WIN32
    close(DRIVER_DATA(pnd)->abort_fds[0]);
    close(DRIVER_DATA(pnd)->abort_fds[1]);
    if (pipe(DRIVER_DATA(pnd)->abort_fds) < 0) {
      return NFC_ESOFT;
    }
#else
    DRIVER_DATA(pnd)->abort_flag = true;
#endif
  }
  return NFC_SUCCESS;
}

const struct pn53x_io acr122s_io = {
  .send    = acr122s_send,
  .receive = acr122s_receive,
};

const struct nfc_driver acr122s_driver = {
  .name       = ACR122S_DRIVER_NAME,
  .scan_type  = INTRUSIVE,
  .scan       = acr122s_scan,
  .open       = acr122s_open,
  .close      = acr122s_close,
  .strerror   = pn53x_strerror,

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

  .abort_command  = acr122s_abort_command,
  .idle           = pn53x_idle,
  /* Even if PN532, PowerDown is not recommended on those devices */
  .powerdown      = NULL,
};
