/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tarti?re
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2013      Laurent Latil
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
 * @file pn532_i2c.c
 * @brief PN532 driver using I2C bus.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "pn532_i2c.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "buses/i2c.h"

#define PN532_I2C_DRIVER_NAME "pn532_i2c"

#define LOG_CATEGORY "libnfc.driver.pn532_i2c"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

// I2C address of the PN532 chip.
#define PN532_I2C_ADDR 0x24

// Internal data structs
const struct pn53x_io pn532_i2c_io;

struct pn532_i2c_data {
  i2c_device dev;
  volatile bool abort_flag;
};

/* Delay for the loop waiting for READY frame (in ms) */
#define PN532_RDY_LOOP_DELAY 90

const struct timespec rdyDelay = {
  .tv_sec = 0,
  .tv_nsec = PN532_RDY_LOOP_DELAY * 1000 * 1000
};

/* Private Functions Prototypes */

static nfc_device *pn532_i2c_open(const nfc_context *context, const nfc_connstring connstring);

static void pn532_i2c_close(nfc_device *pnd);

static int pn532_i2c_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout);

static int pn532_i2c_ack(nfc_device *pnd);

static int pn532_i2c_abort_command(nfc_device *pnd);

static int pn532_i2c_wakeup(nfc_device *pnd);

static int pn532_i2c_wait_rdyframe(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout);

static size_t pn532_i2c_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len);


#define DRIVER_DATA(pnd) ((struct pn532_i2c_data*)(pnd->driver_data))

/**
 * @brief Scan all available I2C buses to find PN532 devices.
 *
 * @param context NFC context.
 * @param connstrings array of 'nfc_connstring' buffer  (allocated by caller). It is used to store the
 *      connection info strings of all I2C PN532 devices found.
 * @param connstrings_len length of the connstrings array.
 * @return number of PN532 devices found on all I2C buses.
 */
static size_t
pn532_i2c_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;
  i2c_device id;
  char **i2cPorts = i2c_list_ports();
  const char *i2cPort;
  int iDevice = 0;

  while ((i2cPort = i2cPorts[iDevice++])) {
    id = i2c_open(i2cPort, PN532_I2C_ADDR);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Trying to find PN532 device on I2C bus %s.", i2cPort);

    if ((id != INVALID_I2C_ADDRESS) && (id != INVALID_I2C_BUS)) {
      nfc_connstring connstring;
      snprintf(connstring, sizeof(nfc_connstring), "%s:%s", PN532_I2C_DRIVER_NAME, i2cPort);
      nfc_device *pnd = nfc_device_new(context, connstring);
      if (!pnd) {
        perror("malloc");
        i2c_close(id);
        iDevice = 0;
        while ((i2cPort = i2cPorts[iDevice++])) {
          free((void *)i2cPort);
        }
        free(i2cPorts);
        return 0;
      }
      pnd->driver = &pn532_i2c_driver;
      pnd->driver_data = malloc(sizeof(struct pn532_i2c_data));
      if (!pnd->driver_data) {
        perror("malloc");
        i2c_close(id);
        nfc_device_free(pnd);
        iDevice = 0;
        while ((i2cPort = i2cPorts[iDevice++])) {
          free((void *)i2cPort);
        }
        free(i2cPorts);
        return 0;
      }
      DRIVER_DATA(pnd)->dev = id;

      // Alloc and init chip's data
      if (pn53x_data_new(pnd, &pn532_i2c_io) == NULL) {
        perror("malloc");
        i2c_close(DRIVER_DATA(pnd)->dev);
        nfc_device_free(pnd);
        iDevice = 0;
        while ((i2cPort = i2cPorts[iDevice++])) {
          free((void *)i2cPort);
        }
        free(i2cPorts);
        return 0;
      }

      // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
      CHIP_DATA(pnd)->type = PN532;
      // This device starts in LowVBat power mode
      CHIP_DATA(pnd)->power_mode = LOWVBAT;

      DRIVER_DATA(pnd)->abort_flag = false;

      // Check communication using "Diagnose" command, with "Communication test" (0x00)
      int res = pn53x_check_communication(pnd);
      i2c_close(DRIVER_DATA(pnd)->dev);
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
  while ((i2cPort = i2cPorts[iDevice++])) {
    free((void *)i2cPort);
  }
  free(i2cPorts);
  return device_found;
}

/**
 * @brief Close I2C connection to the PN532 device.
 *
 * @param pnd pointer on the device to close.
 */
static void
pn532_i2c_close(nfc_device *pnd)
{
  pn53x_idle(pnd);
  i2c_close(DRIVER_DATA(pnd)->dev);

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

/**
 * @brief Open an I2C connection to the PN532 device.
 *
 * @param context NFC context.
 * @param connstring connection info to the device  ( pn532_i2c:<i2c_devname> ).
 * @return pointer to the device, or NULL in case of error.
 */
static nfc_device *
pn532_i2c_open(const nfc_context *context, const nfc_connstring connstring)
{
  char *i2c_devname;
  i2c_device i2c_dev;
  nfc_device *pnd;

  int connstring_decode_level = connstring_decode(connstring, PN532_I2C_DRIVER_NAME, NULL, &i2c_devname, NULL);

  switch (connstring_decode_level) {
    case 2:
      break;
    case 1:
      break;
    case 0:
      return NULL;
  }

  i2c_dev = i2c_open(i2c_devname, PN532_I2C_ADDR);

  if (i2c_dev == INVALID_I2C_BUS || i2c_dev == INVALID_I2C_ADDRESS) {
    return NULL;
  }

  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    i2c_close(i2c_dev);
    return NULL;
  }
  snprintf(pnd->name, sizeof(pnd->name), "%s:%s", PN532_I2C_DRIVER_NAME, i2c_devname);

  pnd->driver_data = malloc(sizeof(struct pn532_i2c_data));
  if (!pnd->driver_data) {
    perror("malloc");
    i2c_close(i2c_dev);
    nfc_device_free(pnd);
    return NULL;
  }
  DRIVER_DATA(pnd)->dev = i2c_dev;

  // Alloc and init chip's data
  if (pn53x_data_new(pnd, &pn532_i2c_io) == NULL) {
    perror("malloc");
    i2c_close(i2c_dev);
    nfc_device_free(pnd);
    return NULL;
  }

  // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
  CHIP_DATA(pnd)->type = PN532;
  // This device starts in LowVBat mode
  CHIP_DATA(pnd)->power_mode = LOWVBAT;

  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 48;
  pnd->driver = &pn532_i2c_driver;

  DRIVER_DATA(pnd)->abort_flag = false;

  // Check communication using "Diagnose" command, with "Communication test" (0x00)
  if (pn53x_check_communication(pnd) < 0) {
    nfc_perror(pnd, "pn53x_check_communication");
    pn532_i2c_close(pnd);
    return NULL;
  }

  pn53x_init(pnd);
  return pnd;
}

static int
pn532_i2c_wakeup(nfc_device *pnd)
{
  /* No specific.  PN532 holds SCL during wakeup time  */
  CHIP_DATA(pnd)->power_mode = NORMAL; // PN532 should now be awake
  return NFC_SUCCESS;
}

#define PN532_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)

/**
 * @brief Send data to the PN532 device.
 *
 * @param pnd pointer on the NFC device.
 * @param pbtData buffer containing frame data.
 * @param szData size of the buffer.
 * @param timeout timeout before aborting the operation (in ms).
 * @return NFC_SUCCESS if operation is successful, or error code.
 */
static int
pn532_i2c_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout)
{
  int res = 0;

  // Discard any existing data ?

  switch (CHIP_DATA(pnd)->power_mode) {
    case LOWVBAT: {
      /** PN532C106 wakeup. */
      if ((res = pn532_i2c_wakeup(pnd)) < 0) {
        return res;
      }
      // According to PN532 application note, C106 appendix: to go out Low Vbat mode and enter in normal mode we need to send a SAMConfiguration command
      if ((res = pn532_SAMConfiguration(pnd, PSM_NORMAL, 1000)) < 0) {
        return res;
      }
    }
    break;
    case POWERDOWN: {
      if ((res = pn532_i2c_wakeup(pnd)) < 0) {
        return res;
      }
    }
    break;
    case NORMAL:
      // Nothing to do :)
      break;
  };

  uint8_t abtFrame[PN532_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  size_t szFrame = 0;

  if ((res = pn53x_build_frame(abtFrame, &szFrame, pbtData, szData)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }

  res = i2c_write(DRIVER_DATA(pnd)->dev, abtFrame, szFrame);

  if (res < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->last_error = res;
    return pnd->last_error;
  }

  uint8_t abtRxBuf[PN53x_ACK_FRAME__LEN];

  // Wait for the ACK frame
  res = pn532_i2c_wait_rdyframe(pnd, abtRxBuf, sizeof(abtRxBuf), timeout);
  if (res < 0) {
    if (res == NFC_EOPABORTED) {
      // Send an ACK frame from host to abort the command.
      pn532_i2c_ack(pnd);
    }
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (pn53x_check_ack_frame(pnd, abtRxBuf, res) == 0) {
    // The PN53x is running the sent command
  } else {
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

/**
 * @brief Read data from the PN532 device until getting a frame with RDY bit set
 *
 * @param pnd pointer on the NFC device.
 * @param pbtData buffer used to store the received frame data.
 * @param szDataLen allocated size of buffer.
 * @param timeout timeout delay before aborting the operation (in ms). Use 0 for no timeout.
 * @return length (in bytes) of the received frame, or NFC_ETIMEOUT if timeout delay has expired,
 *         NFC_EOPABORTED if operation has been aborted, NFC_EIO in case of IO failure
 */
static int
pn532_i2c_wait_rdyframe(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout)
{
  bool done = false;
  int res;

  struct timeval start_tv, cur_tv;
  long long duration;

  // Actual I2C response frame includes an additional status byte,
  // so we use a temporary buffer to read the I2C frame
  uint8_t i2cRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN + 1];

  if (timeout > 0) {
    // If a timeout is specified, get current timestamp
    gettimeofday(&start_tv, NULL);
  }

  do {
    // Wait a little bit before reading
    nanosleep(&rdyDelay, (struct timespec *) NULL);

    int recCount = i2c_read(DRIVER_DATA(pnd)->dev, i2cRx, szDataLen + 1);

    if (DRIVER_DATA(pnd)->abort_flag) {
      // Reset abort flag
      DRIVER_DATA(pnd)->abort_flag = false;

      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
              "Wait for a READY frame has been aborted.");
      return NFC_EOPABORTED;
    }

    if (recCount <= 0) {
      done = true;
      res = NFC_EIO;
    } else {
      const uint8_t rdy = i2cRx[0];
      if (rdy & 1) {
        int copyLength;

        done = true;
        res = recCount - 1;
        copyLength = MIN(res, (int)szDataLen);
        memcpy(pbtData, &(i2cRx[1]), copyLength);
      } else {
        /* Not ready yet. Check for elapsed timeout. */

        if (timeout > 0) {
          gettimeofday(&cur_tv, NULL);
          duration = (cur_tv.tv_sec - start_tv.tv_sec) * 1000000L
                     + (cur_tv.tv_usec - start_tv.tv_usec);

          if (duration / 1000 > timeout) {
            res = NFC_ETIMEOUT;
            done = true;

            log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
                    "timeout reached with no READY frame.");
          }
        }
      }
    }
  } while (!done);

  return res;
}

/**
 * @brief Read a response frame from the PN532 device.
 *
 * @param pnd pointer on the NFC device.
 * @param pbtData buffer used to store the response frame data.
 * @param szDataLen allocated size of buffer.
 * @param timeout timeout delay before aborting the operation (in ms). Use 0 for no timeout.
 * @return length (in bytes) of the response, or NFC_ETIMEOUT if timeout delay has expired,
 *         NFC_EOPABORTED if operation has been aborted, NFC_EIO in case of IO failure
 */
static int
pn532_i2c_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout)
{
  uint8_t frameBuf[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  int frameLength;
  int TFI_idx;
  size_t len;

  frameLength = pn532_i2c_wait_rdyframe(pnd, frameBuf, sizeof(frameBuf), timeout);

  if (NFC_EOPABORTED == pnd->last_error) {
    return pn532_i2c_ack(pnd);
  }

  if (frameLength < 0) {
    goto error;
  }

  const uint8_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };

  if (0 != (memcmp(frameBuf, pn53x_preamble, 3))) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame preamble+start code mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if ((0x01 == frameBuf[3]) && (0xff == frameBuf[4])) {
    uint8_t errorCode = frameBuf[5];
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Application level error detected  (%d)", errorCode);
    pnd->last_error = NFC_EIO;
    goto error;
  } else if ((0xff == frameBuf[3]) && (0xff == frameBuf[4])) {
    // Extended frame
    len = (frameBuf[5] << 8) + frameBuf[6];

    // Verify length checksum
    if (((frameBuf[5] + frameBuf[6] + frameBuf[7]) % 256) != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }
    TFI_idx = 8;
  } else {
    // Normal frame

    len = frameBuf[3];

    // Verify length checksum
    if ((uint8_t)(frameBuf[3] + frameBuf[4])) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }
    TFI_idx = 5;
  }

  if ((len - 2) > szDataLen) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %" PRIuPTR ", len: %" PRIuPTR ")", szDataLen, len);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  uint8_t TFI = frameBuf[TFI_idx];
  if (TFI != 0xD5) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (frameBuf[TFI_idx + 1] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Command Code verification failed.  (got %d,  expected %d)",
            frameBuf[TFI_idx + 1], CHIP_DATA(pnd)->last_command + 1);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  uint8_t DCS = frameBuf[TFI_idx + len];
  uint8_t btDCS = DCS;

  // Compute data checksum
  for (size_t i = 0; i < len; i++) {
    btDCS += frameBuf[TFI_idx + i];
  }

  if (btDCS != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Data checksum mismatch  (DCS = %02x, btDCS = %d)", DCS, btDCS);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (0x00 != frameBuf[TFI_idx + len + 1]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Frame postamble mismatch  (got %d)", frameBuf[frameLength - 1]);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  memcpy(pbtData, &frameBuf[TFI_idx + 2], len - 2);

  /* The PN53x command is done and we successfully received the reply */
  return len - 2;
error:
  return pnd->last_error;
}

/**
 * @brief Send an ACK frame to the PN532 device.
 *
 * @param pnd pointer on the NFC device.
 * @return NFC_SUCCESS on success, otherwise an error code
 */
int
pn532_i2c_ack(nfc_device *pnd)
{
  return i2c_write(DRIVER_DATA(pnd)->dev, pn53x_ack_frame, sizeof(pn53x_ack_frame));
}

/**
 * @brief Abort any pending operation
 *
 * @param pnd pointer on the NFC device.
 * @return NFC_SUCCESS
 */
static int
pn532_i2c_abort_command(nfc_device *pnd)
{
  if (pnd) {
    DRIVER_DATA(pnd)->abort_flag = true;
  }
  return NFC_SUCCESS;
}

const struct pn53x_io pn532_i2c_io = {
  .send       = pn532_i2c_send,
  .receive    = pn532_i2c_receive,
};

const struct nfc_driver pn532_i2c_driver = {
  .name                             = PN532_I2C_DRIVER_NAME,
  .scan_type                        = INTRUSIVE,
  .scan                             = pn532_i2c_scan,
  .open                             = pn532_i2c_open,
  .close                            = pn532_i2c_close,
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

  .abort_command  = pn532_i2c_abort_command,
  .idle           = pn53x_idle,
  .powerdown      = pn53x_PowerDown,
};

