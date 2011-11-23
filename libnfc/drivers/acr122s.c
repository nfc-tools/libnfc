/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2011, Anugrah Redja Kusuma <anugrah.redja@gmail.com>
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

struct acr122s_data {
  serial_port port;
  byte_t seq;
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
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t power_select;
  byte_t rfu[2];
};

struct icc_power_on_res {
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t status;
  byte_t error;
  byte_t chain_parameter;
};

struct icc_power_off_req {
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t rfu[3];
};

struct icc_power_off_res {
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t status;
  byte_t error;
  byte_t clock_status;
};

struct xfr_block_req {
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t bwi;
  byte_t rfu[2];
};

struct xfr_block_res {
  byte_t message_type;
  uint32_t length;
  byte_t slot;
  byte_t seq;
  byte_t status;
  byte_t error;
  byte_t chain_parameter;
};

struct apdu_header {
  byte_t class;
  byte_t ins;
  byte_t p1;
  byte_t p2;
  byte_t length;
};

#pragma pack(pop)

#define TRACE do { printf("%s:%d\n", __func__, __LINE__); } while (0)

#define DRIVER_DATA(dev) ((struct acr122s_data *) (dev->driver_data))

/**
 * Print a debuggin hex string to stdout.
 *
 * @param caption is buffer label
 * @param buf is buffer to be print
 * @param buf_len is buffer length
 */
#if 0
static void
print_hex(const char *caption, byte_t *buf, size_t buf_len)
{
  printf("%s:", caption);
  for (size_t i = 0; i < buf_len; i++) {
    printf(" %02x", buf[i]);
  }
  puts("");
}
#endif

/**
 * Fix a command frame with a valid prefix, checksum, and suffix.
 *
 * @param frame is command frame to fix
 * @note command frame length (uint32_t at offset 2) should be valid
 */
static void
acr122s_fix_frame(byte_t *frame)
{
  size_t frame_size = FRAME_SIZE(frame);
  frame[0] = STX;
  frame[frame_size - 1] = ETX;

  byte_t *csum = frame + frame_size - 2;
  *csum = 0;
  for (byte_t *p = frame + 1; p < csum; p++)
    *csum ^= *p;
}

/**
 * Send a command frame to ACR122S and check its ACK status.
 *
 * @param: dev is target nfc device
 * @param: cmd is command frame to send
 * @param: timeout
 * @return 0 if success
 */
static int
acr122s_send_frame(nfc_device_t *dev, byte_t *frame, struct timeval *timeout)
{
  size_t frame_size = FRAME_SIZE(frame);
  byte_t ack[4];
  byte_t positive_ack[4] = { STX, 0, 0, ETX };
  serial_port port = DRIVER_DATA(dev)->port;
  int ret;
  void *abort_p;

#ifndef WIN32
  abort_p = &(DRIVER_DATA(dev)->abort_fds[1]);
#else
  abort_p = &(DRIVER_DATA(dev)->abort_flag);
#endif

  if ((ret = uart_send(port, frame, frame_size, timeout)) != 0)
    return ret;

  if ((ret = uart_receive(port, ack, 4, abort_p, timeout)) != 0)
    return ret;

  if (memcmp(ack, positive_ack, 4) != 0)
    return ECOMIO;

  struct xfr_block_req *req = (struct xfr_block_req *) &frame[1];
  DRIVER_DATA(dev)->seq = req->seq + 1;

  return 0;
}

/**
 * Receive response frame after a successfull acr122s_send_command().
 *
 * @param: dev is target nfc device
 * @param: frame is buffer where received response frame will be stored
 * @param: frame_size is frame size
 * @param: abort_p
 * @param: timeout
 * @note returned frame size can be fetched using FRAME_SIZE macro
 *
 * @return 0 if success
 */
static int
acr122s_recv_frame(nfc_device_t *dev, byte_t *frame, size_t frame_size, void *abort_p, struct timeval *timeout)
{
  if (frame_size < 13)
    return EINVALARG;

  int ret;
  serial_port port = DRIVER_DATA(dev)->port;

  if ((ret = uart_receive(port, frame, 11, abort_p, timeout)) != 0)
    return ret;

  // Is buffer sufficient to store response?
  if (frame_size < FRAME_SIZE(frame))
    return ECOMIO;

  size_t remaining = FRAME_SIZE(frame) - 11;
  if ((ret = uart_receive(port, frame + 11, remaining, abort_p, timeout)) != 0)
    return ret;

  struct xfr_block_res *res = (struct xfr_block_res *) &frame[1];
  if ((byte_t) (res->seq + 1) != DRIVER_DATA(dev)->seq) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Invalid response sequence number.");
    return ECOMIO;
  }

  return 0;
}

#define APDU_OVERHEAD (FRAME_OVERHEAD + 5)

/**
 * Convert host uint32 to litle endian uint32
 */
static uint32_t
le32(uint32_t val) {
	uint32_t res;
	byte_t *p = (byte_t *) &res;
	p[0] = val;
	p[1] = val >> 8;
	p[2] = val >> 16;
	p[3] = val >> 24;
	return res;
}

/**
 * Build an ACR122S command frame from a PN532 command.
 *
 * @param dev is device for which the command frame will be generated
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
acr122s_build_frame(nfc_device_t *dev,
    byte_t *frame, size_t frame_size, byte_t p1, byte_t p2,
    const byte_t *data, size_t data_size, int should_prefix)
{
  if (frame_size < data_size + APDU_OVERHEAD + should_prefix)
    return false;
  if (data_size + should_prefix > 255)
    return false;

  struct xfr_block_req *req = (struct xfr_block_req *) &frame[1];
  req->message_type = XFR_BLOCK_REQ_MSG;
  req->length = le32(5 + data_size + should_prefix);
  req->slot = 0;
  req->seq = DRIVER_DATA(dev)->seq;
  req->bwi = 0;
  req->rfu[0] = 0;
  req->rfu[1] = 0;

  struct apdu_header *header = (struct apdu_header *) &frame[11];
  header->class = 0xff;
  header->ins = 0;
  header->p1 = p1;
  header->p2 = p2;
	header->length = data_size + should_prefix;

  byte_t *buf = (byte_t *) &frame[16];
  if (should_prefix)
    *buf++ = 0xD4;
  memcpy(buf, data, data_size);
  acr122s_fix_frame(frame);

  return true;
}

static int
acr122s_activate_sam(nfc_device_t *dev)
{
  byte_t cmd[13];
  memset(cmd, 0, sizeof(cmd));
  cmd[1] = ICC_POWER_ON_REQ_MSG;
  acr122s_fix_frame(cmd);

  byte_t resp[MAX_FRAME_SIZE];
  int ret;

  if ((ret = acr122s_send_frame(dev, cmd, 0)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(dev, resp, MAX_FRAME_SIZE, 0, 0)) != 0)
    return ret;

  CHIP_DATA(dev)->power_mode = NORMAL;

  return 0;
}

static int
acr122s_deactivate_sam(nfc_device_t *dev)
{
  byte_t cmd[13];
  memset(cmd, 0, sizeof(cmd));
  cmd[1] = ICC_POWER_OFF_REQ_MSG;
  acr122s_fix_frame(cmd);

  byte_t resp[MAX_FRAME_SIZE];
  int ret;

  if ((ret = acr122s_send_frame(dev, cmd, 0)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(dev, resp, MAX_FRAME_SIZE, 0, 0)) != 0)
    return ret;

  CHIP_DATA(dev)->power_mode = LOWVBAT;

  return 0;
}

static int
acr122s_get_firmware_version(nfc_device_t *dev, char *version, size_t length)
{
  int ret;
  byte_t cmd[MAX_FRAME_SIZE];

  acr122s_build_frame(dev, cmd, sizeof(cmd), 0x48, 0, NULL, 0, 0);

  if ((ret = acr122s_send_frame(dev, cmd, 0)) != 0)
    return ret;

  if ((ret = acr122s_recv_frame(dev, cmd, sizeof(cmd), 0, 0)) != 0)
    return ret;

  size_t len = APDU_SIZE(cmd);
  if (len + 1 > length)
    len = length - 1;
  memcpy(version, cmd + 11, len);
  version[len] = 0;

  return 0;
}

bool
acr122s_probe(nfc_device_desc_t descs[], size_t desc_count, size_t *dev_found)
{
  /** @note: Due to UART bus we can't know if its really an ACR122S without
  * sending some commands. But using this way to probe devices, we can
  * have serious problem with other device on this bus */
#ifndef SERIAL_AUTOPROBE_ENABLED
  (void) descs;
  (void) desc_count;
  *dev_found = 0;
  log_put(LOG_CATEGORY, NFC_PRIORITY_INFO, "Serial auto-probing have been disabled at compile time. Skipping autoprobe.");
  return false;
#else /* SERIAL_AUTOPROBE_ENABLED */
  *dev_found = 0;
  char **ports = uart_list_ports();
  for (int i = 0; ports[i]; i++) {
    char *port = ports[i];
    serial_port sp = uart_open(port);
    log_put (LOG_CATEGORY, NFC_PRIORITY_TRACE, "Trying to find ACR122S device on serial port: %s at %d bauds.", port, ACR122S_DEFAULT_SPEED);

    if ((sp != INVALID_SERIAL_PORT) && (sp != CLAIMED_SERIAL_PORT)) {
      uart_flush_input(sp);
      uart_set_speed(sp, ACR122S_DEFAULT_SPEED);

      nfc_device_t *dev = nfc_device_new();
      dev->driver = &acr122s_driver;

      dev->driver_data = malloc(sizeof(struct acr122s_data));
      DRIVER_DATA(dev)->port = sp;
      DRIVER_DATA(dev)->seq = 0;

#ifndef WIN32
      pipe(DRIVER_DATA(dev)->abort_fds);
#else
      DRIVER_DATA(dev)->abort_flag = false;
#endif

      pn53x_data_new(dev, &acr122s_io);
      CHIP_DATA(dev)->type = PN532;
      CHIP_DATA(dev)->power_mode = NORMAL;

      char version[32];
      int ret = acr122s_get_firmware_version(dev, version, sizeof(version));
      if (ret == 0 && strncmp("ACR122S", version, 7) != 0) {
        ret = -1;
      }

      pn53x_data_free(dev);
      nfc_device_free(dev);
      uart_close(sp);

      if (ret != 0)
        continue;

      nfc_device_desc_t *desc = &descs[(*dev_found)++];
      snprintf(desc->acDevice, DEVICE_NAME_LENGTH - 1, "%s (%s)", ACR122S_DRIVER_NAME, port);
      desc->pcDriver = ACR122S_DRIVER_NAME;
      strncpy(desc->acPort, port, DEVICE_PORT_LENGTH - 1);
      desc->acPort[DEVICE_PORT_LENGTH - 1] = '\0';
      desc->uiSpeed = ACR122S_DEFAULT_SPEED;

      // Test if we reach the maximum "wanted" devices
      if (*dev_found >= desc_count)
        break;
    }
  }

  for (int i = 0; ports[i]; i++)
    free(ports[i]);
  free(ports);

  return true;
#endif /* SERIAL_AUTOPROBE_ENABLED */
}

nfc_device_t *
acr122s_connect(const nfc_device_desc_t *desc)
{
  serial_port sp;
  nfc_device_t *dev;

  log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE,
      "Attempt to connect to: %s at %d bauds.", desc->acPort, desc->uiSpeed);

  sp = uart_open(desc->acPort);
  if (sp == INVALID_SERIAL_PORT) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR,
        "Invalid serial port: %s", desc->acPort);
    return NULL;
  }
  if (sp == CLAIMED_SERIAL_PORT) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR,
        "Serial port already claimed: %s", desc->acPort);
    return NULL;
  }

  uart_flush_input(sp);
  uart_set_speed(sp, desc->uiSpeed);

  dev = nfc_device_new();
  dev->driver = &acr122s_driver;
  strcpy(dev->acName, ACR122S_DRIVER_NAME);

  dev->driver_data = malloc(sizeof(struct acr122s_data));
  DRIVER_DATA(dev)->port = sp;
  DRIVER_DATA(dev)->seq = 0;

#ifndef WIN32
  pipe(DRIVER_DATA(dev)->abort_fds);
#else
  DRIVER_DATA(dev)->abort_flag = false;
#endif

  pn53x_data_new(dev, &acr122s_io);
  CHIP_DATA(dev)->type = PN532;

#if 1
  // Retrieve firmware version
  char version[DEVICE_NAME_LENGTH];
  if (acr122s_get_firmware_version(dev, version, sizeof(version)) != 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Cannot get reader firmware.");
    acr122s_disconnect(dev);
    return NULL;
  }

  if (strncmp(version, "ACR122S", 7) != 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Invalid firmware version: %s",
        version);
    acr122s_disconnect(dev);
    return NULL;
  }

  snprintf(dev->acName, sizeof(dev->acName), "%s", version);

  // Activate SAM before operating
  if (acr122s_activate_sam(dev) != 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Cannot activate SAM.");
    acr122s_disconnect(dev);
    return NULL;
  }
#endif

  if (!pn53x_init(dev)) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Failed initializing PN532 chip.");
    acr122s_disconnect(dev);
    return NULL;
  }

  return dev;
}

void
acr122s_disconnect (nfc_device_t *dev)
{
  acr122s_deactivate_sam(dev);
  uart_close(DRIVER_DATA(dev)->port);

#ifndef WIN32
  // Release file descriptors used for abort mecanism
  close (DRIVER_DATA(dev)->abort_fds[0]);
  close (DRIVER_DATA(dev)->abort_fds[1]);
#endif

  pn53x_data_free(dev);
  nfc_device_free(dev);
}

bool
acr122s_send(nfc_device_t *dev, const byte_t *buf, const size_t buf_len, struct timeval *timeout)
{
  uart_flush_input(DRIVER_DATA(dev)->port);

  byte_t cmd[MAX_FRAME_SIZE];
  acr122s_build_frame(dev, cmd, sizeof(cmd), 0, 0, buf, buf_len, 1);
  int ret;
  if ((ret = acr122s_send_frame(dev, cmd, timeout)) != 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    dev->iLastError = ret;
    return false;
  }

  return true;
}

int
acr122s_receive(nfc_device_t *dev, byte_t *buf, size_t buf_len, struct timeval *timeout)
{
  void *abort_p;

#ifndef WIN32
  abort_p = &(DRIVER_DATA(dev)->abort_fds[1]);
#else
  abort_p = &(DRIVER_DATA(dev)->abort_flag);
#endif

  byte_t tmp[MAX_FRAME_SIZE];
  dev->iLastError = acr122s_recv_frame(dev, tmp, sizeof(tmp), abort_p, timeout);

  if (abort_p && (EOPABORT == dev->iLastError))
    return -1;

  if (dev->iLastError != 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    return -1;
  }

  size_t data_len = FRAME_SIZE(tmp) - 17;
  if (data_len > buf_len) {
    log_put (LOG_CATEGORY, NFC_PRIORITY_ERROR, "Receive buffer too small. (buf_len: %zu, data_len: %zu)", buf_len, data_len);
    dev->iLastError = ECOMIO;
    return -1;
  }

  memcpy(buf, tmp + 13, data_len);
  return data_len;
}

bool
acr122s_abort_command(nfc_device_t *dev)
{
  if (dev) {
#ifndef WIN32
    close(DRIVER_DATA(dev)->abort_fds[0]);
    close(DRIVER_DATA(dev)->abort_fds[1]);
    pipe(DRIVER_DATA(dev)->abort_fds);
#else
    DRIVER_DATA(dev)->abort_flag = true;
#endif
  }
  return true;
}

const struct pn53x_io acr122s_io = {
  .send    = acr122s_send,
  .receive = acr122s_receive,
};

const struct nfc_driver_t acr122s_driver = {
  .name       = ACR122S_DRIVER_NAME,
  .probe      = acr122s_probe,
  .connect    = acr122s_connect,
  .disconnect = acr122s_disconnect,
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

  .abort_command  = acr122s_abort_command,
  .idle  = NULL,
};
