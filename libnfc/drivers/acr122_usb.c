/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romain Tartière, Romuald Conty
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
 * @file acr122_usb.c
 * @brief Driver for ACR122 using direct USB (without PCSC)
 */

/*
 * This implementation was written based on information provided by the
 * following documents:
 *
 * Smart Card CCID
 * Specification for Integrated Circuit(s) Cards Interface Devices
 * Revision 1.1
 * April 22rd, 2005
 * http://www.usb.org/developers/devclass_docs/DWG_Smart-Card_CCID_Rev110.pdf
 *
 * ACR122U NFC Reader
 * Application Programming Interface
 * Revision 1.2
 * http://acs.com.hk/drivers/eng/API_ACR122U.pdf
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

/*
Thanks to d18c7db and Okko for example code
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <errno.h>

#ifndef _WIN32
// Under POSIX system, we use libusb (>= 0.1.12)
#include <usb.h>
#define USB_TIMEDOUT ETIMEDOUT
#define _usb_strerror( X ) strerror(-X)
#else
// Under Windows we use libusb-win32 (>= 1.2.5)
#include <lusb0_usb.h>
#define USB_TIMEDOUT 116
#define _usb_strerror( X ) usb_strerror()
#endif

#include <string.h>

#include <nfc/nfc.h>

#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "drivers/acr122_usb.h"

#define ACR122_USB_DRIVER_NAME "acr122_usb"
#define LOG_CATEGORY "libnfc.driver.acr122_usb"

#define USB_INFINITE_TIMEOUT   0

#define DRIVER_DATA(pnd) ((struct acr122_usb_data*)(pnd->driver_data))

typedef enum {
  UNKNOWN,
  ACR122,
  TOUCHATAG,
} acr122_usb_model;

struct acr122_usb_data {
  usb_dev_handle *pudh;
  acr122_usb_model model;
  uint32_t uiEndPointIn;
  uint32_t uiEndPointOut;
  uint32_t uiMaxPacketSize;
  volatile bool abort_flag;
};

const struct pn53x_io acr122_usb_io;
bool acr122_usb_get_usb_device_name(struct usb_device *dev, usb_dev_handle *udev, char *buffer, size_t len);
int  acr122_usb_init(nfc_device *pnd);
int  acr122_usb_ack(nfc_device *pnd);

static int
acr122_usb_bulk_read(struct acr122_usb_data *data, uint8_t abtRx[], const size_t szRx, const int timeout)
{
  int res = usb_bulk_read(data->pudh, data->uiEndPointIn, (char *) abtRx, szRx, timeout);
  if (res > 0) {
    LOG_HEX("RX", abtRx, res);
  } else if (res < 0) {
    if (res != -USB_TIMEDOUT) {
      res = NFC_EIO;
      log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to read from USB (%s)", _usb_strerror(res));
    } else {
      res = NFC_ETIMEOUT;
    }
  }
  return res;
}

static int
acr122_usb_bulk_write(struct acr122_usb_data *data, uint8_t abtTx[], const size_t szTx, const int timeout)
{
  LOG_HEX("TX", abtTx, szTx);
  int res = usb_bulk_write(data->pudh, data->uiEndPointOut, (char *) abtTx, szTx, timeout);
  if (res > 0) {
    // HACK This little hack is a well know problem of USB, see http://www.libusb.org/ticket/6 for more details
    if ((res % data->uiMaxPacketSize) == 0) {
      usb_bulk_write(data->pudh, data->uiEndPointOut, "\0", 0, timeout);
    }
  } else if (res < 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to write to USB (%s)", _usb_strerror(res));
    if (res == -USB_TIMEDOUT) {
      res = NFC_ETIMEOUT;
    } else {
      res = NFC_EIO;
    }
  }
  return res;
}

struct acr122_usb_supported_device {
  uint16_t vendor_id;
  uint16_t product_id;
  acr122_usb_model model;
  const char *name;
};

const struct acr122_usb_supported_device acr122_usb_supported_devices[] = {
  { 0x072F, 0x2200, ACR122,   "ACS ACR122" },
  { 0x072F, 0x90CC, TOUCHATAG,   "Touchatag" },
};

static acr122_usb_model
acr122_usb_get_device_model(uint16_t vendor_id, uint16_t product_id)
{
  for (size_t n = 0; n < sizeof(acr122_usb_supported_devices) / sizeof(struct acr122_usb_supported_device); n++) {
    if ((vendor_id == acr122_usb_supported_devices[n].vendor_id) &&
        (product_id == acr122_usb_supported_devices[n].product_id))
      return acr122_usb_supported_devices[n].model;
  }

  return UNKNOWN;
}

// Find transfer endpoints for bulk transfers
static void
acr122_usb_get_end_points(struct usb_device *dev, struct acr122_usb_data *data)
{
  uint32_t uiIndex;
  uint32_t uiEndPoint;
  struct usb_interface_descriptor *puid = dev->config->interface->altsetting;

  // 3 Endpoints maximum: Interrupt In, Bulk In, Bulk Out
  for (uiIndex = 0; uiIndex < puid->bNumEndpoints; uiIndex++) {
    // Only accept bulk transfer endpoints (ignore interrupt endpoints)
    if (puid->endpoint[uiIndex].bmAttributes != USB_ENDPOINT_TYPE_BULK)
      continue;

    // Copy the endpoint to a local var, makes it more readable code
    uiEndPoint = puid->endpoint[uiIndex].bEndpointAddress;

    // Test if we dealing with a bulk IN endpoint
    if ((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN) {
      data->uiEndPointIn = uiEndPoint;
      data->uiMaxPacketSize = puid->endpoint[uiIndex].wMaxPacketSize;
    }
    // Test if we dealing with a bulk OUT endpoint
    if ((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT) {
      data->uiEndPointOut = uiEndPoint;
      data->uiMaxPacketSize = puid->endpoint[uiIndex].wMaxPacketSize;
    }
  }
}

bool
acr122_usb_probe(nfc_connstring connstrings[], size_t connstrings_len, size_t *pszDeviceFound)
{
  usb_init();

  int res;
  // usb_find_busses will find all of the busses on the system. Returns the
  // number of changes since previous call to this function (total of new
  // busses and busses removed).
  if ((res = usb_find_busses() < 0)) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to find USB busses (%s)", _usb_strerror(res));
    return false;
  }
  // usb_find_devices will find all of the devices on each bus. This should be
  // called after usb_find_busses. Returns the number of changes since the
  // previous call to this function (total of new device and devices removed).
  if ((res = usb_find_devices() < 0)) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to find USB devices (%s)", _usb_strerror(res));
    return false;
  }

  *pszDeviceFound = 0;

  uint32_t uiBusIndex = 0;
  struct usb_bus *bus;
  for (bus = usb_get_busses(); bus; bus = bus->next) {
    struct usb_device *dev;

    for (dev = bus->devices; dev; dev = dev->next, uiBusIndex++) {
      for (size_t n = 0; n < sizeof(acr122_usb_supported_devices) / sizeof(struct acr122_usb_supported_device); n++) {
        if ((acr122_usb_supported_devices[n].vendor_id == dev->descriptor.idVendor) &&
            (acr122_usb_supported_devices[n].product_id == dev->descriptor.idProduct)) {
          // Make sure there are 2 endpoints available
          // with libusb-win32 we got some null pointers so be robust before looking at endpoints:
          if (dev->config == NULL || dev->config->interface == NULL || dev->config->interface->altsetting == NULL) {
            // Nope, we maybe want the next one, let's try to find another
            continue;
          }
          if (dev->config->interface->altsetting->bNumEndpoints < 2) {
            // Nope, we maybe want the next one, let's try to find another
            continue;
          }

          usb_dev_handle *udev = usb_open(dev);

          // Set configuration
          // acr122_usb_get_usb_device_name (dev, udev, pnddDevices[*pszDeviceFound].acDevice, sizeof (pnddDevices[*pszDeviceFound].acDevice));
          log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "device found: Bus %s Device %s", bus->dirname, dev->filename);
          usb_close(udev);
          snprintf(connstrings[*pszDeviceFound], sizeof(nfc_connstring), "%s:%s:%s", ACR122_USB_DRIVER_NAME, bus->dirname, dev->filename);
          (*pszDeviceFound)++;
          // Test if we reach the maximum "wanted" devices
          if ((*pszDeviceFound) == connstrings_len) {
            return true;
          }
        }
      }
    }
  }

  return true;
}

struct acr122_usb_descriptor {
  char *dirname;
  char *filename;
};

static int
acr122_usb_connstring_decode(const nfc_connstring connstring, struct acr122_usb_descriptor *desc)
{
  int n = strlen(connstring) + 1;
  char *driver_name = malloc(n);
  char *dirname     = malloc(n);
  char *filename    = malloc(n);

  driver_name[0] = '\0';

  int res = sscanf(connstring, "%[^:]:%[^:]:%[^:]", driver_name, dirname, filename);

  if (!res || ((0 != strcmp(driver_name, ACR122_USB_DRIVER_NAME)) && (0 != strcmp(driver_name, "usb")))) {
    // Driver name does not match.
    res = 0;
  } else {
    desc->dirname  = strdup(dirname);
    desc->filename = strdup(filename);
  }

  free(driver_name);
  free(dirname);
  free(filename);

  return res;
}

bool
acr122_usb_get_usb_device_name(struct usb_device *dev, usb_dev_handle *udev, char *buffer, size_t len)
{
  *buffer = '\0';

  if (dev->descriptor.iManufacturer || dev->descriptor.iProduct) {
    if (udev) {
      usb_get_string_simple(udev, dev->descriptor.iManufacturer, buffer, len);
      if (strlen(buffer) > 0)
        strcpy(buffer + strlen(buffer), " / ");
      usb_get_string_simple(udev, dev->descriptor.iProduct, buffer + strlen(buffer), len - strlen(buffer));
    }
  }

  if (!*buffer) {
    for (size_t n = 0; n < sizeof(acr122_usb_supported_devices) / sizeof(struct acr122_usb_supported_device); n++) {
      if ((acr122_usb_supported_devices[n].vendor_id == dev->descriptor.idVendor) &&
          (acr122_usb_supported_devices[n].product_id == dev->descriptor.idProduct)) {
        strncpy(buffer, acr122_usb_supported_devices[n].name, len);
        return true;
      }
    }
  }

  return false;
}

static nfc_device *
acr122_usb_open(const nfc_connstring connstring)
{
  nfc_device *pnd = NULL;
  struct acr122_usb_descriptor desc = { NULL, NULL };
  int connstring_decode_level = acr122_usb_connstring_decode(connstring, &desc);
  log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "%d element(s) have been decoded from \"%s\"", connstring_decode_level, connstring);
  if (connstring_decode_level < 1) {
    goto free_mem;
  }

  struct acr122_usb_data data = {
    .pudh = NULL,
    .uiEndPointIn = 0,
    .uiEndPointOut = 0,
  };
  struct usb_bus *bus;
  struct usb_device *dev;

  usb_init();

  int res;
  // usb_find_busses will find all of the busses on the system. Returns the
  // number of changes since previous call to this function (total of new
  // busses and busses removed).
  if ((res = usb_find_busses() < 0)) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to find USB busses (%s)", _usb_strerror(res));
    goto free_mem;
  }
  // usb_find_devices will find all of the devices on each bus. This should be
  // called after usb_find_busses. Returns the number of changes since the
  // previous call to this function (total of new device and devices removed).
  if ((res = usb_find_devices() < 0)) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to find USB devices (%s)", _usb_strerror(res));
    goto free_mem;
  }

  for (bus = usb_get_busses(); bus; bus = bus->next) {
    if (connstring_decode_level > 1)  {
      // A specific bus have been specified
      if (0 != strcmp(bus->dirname, desc.dirname))
        continue;
    }
    for (dev = bus->devices; dev; dev = dev->next) {
      if (connstring_decode_level > 2)  {
        // A specific dev have been specified
        if (0 != strcmp(dev->filename, desc.filename))
          continue;
      }
      // Open the USB device
      data.pudh = usb_open(dev);
      // Reset device
      usb_reset(data.pudh);
      // Retrieve end points
      acr122_usb_get_end_points(dev, &data);
      // Claim interface
      res = usb_claim_interface(data.pudh, 0);
      if (res < 0) {
        log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to claim USB interface (%s)", _usb_strerror(res));
        usb_close(data.pudh);
        // we failed to use the specified device
        goto free_mem;
      }

      res = usb_set_altinterface(data.pudh, 0);
      if (res < 0) {
        log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to set alternate setting on USB interface (%s)", _usb_strerror(res));
        usb_close(data.pudh);
        // we failed to use the specified device
        goto free_mem;
      }

      data.model = acr122_usb_get_device_model(dev->descriptor.idVendor, dev->descriptor.idProduct);
      // Allocate memory for the device info and specification, fill it and return the info
      pnd = nfc_device_new(connstring);
      acr122_usb_get_usb_device_name(dev, data.pudh, pnd->name, sizeof(pnd->name));

      pnd->driver_data = malloc(sizeof(struct acr122_usb_data));
      *DRIVER_DATA(pnd) = data;

      // Alloc and init chip's data
      pn53x_data_new(pnd, &acr122_usb_io);

      switch (DRIVER_DATA(pnd)->model) {
          // empirical tuning
        case ACR122:
          CHIP_DATA(pnd)->timer_correction = 46;
          break;
        case TOUCHATAG:
          CHIP_DATA(pnd)->timer_correction = 50;
          break;
        case UNKNOWN:
          break;
      }
      pnd->driver = &acr122_usb_driver;

      if (acr122_usb_init(pnd) < 0) {
        usb_close(data.pudh);
        goto error;
      }
      DRIVER_DATA(pnd)->abort_flag = false;
      goto free_mem;
    }
  }
  // We ran out of devices before the index required
  goto free_mem;

error:
  // Free allocated structure on error.
  nfc_device_free(pnd);
  pnd = NULL;
free_mem:
  free(desc.dirname);
  free(desc.filename);
  return pnd;
}

static void
acr122_usb_close(nfc_device *pnd)
{
  acr122_usb_ack(pnd);

  pn53x_idle(pnd);

  int res;
  if ((res = usb_release_interface(DRIVER_DATA(pnd)->pudh, 0)) < 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to release USB interface (%s)", _usb_strerror(res));
  }

  if ((res = usb_close(DRIVER_DATA(pnd)->pudh)) < 0) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to close USB connection (%s)", _usb_strerror(res));
  }
  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

/*
USB activity trace for PN533, ACR122 and Touchatag

--------------------------------------------------------------------
PN533
                     0000ff02fe d402          2a00
                     0000ff00ff00
                     ACK
                     0000ff06fa d50333020707  e500

--------------------------------------------------------------------
Acr122U PICC    pseudo-APDU through PCSC Escape mechanism:
6b07000000000a000000 ff00000002 d402
PC_to_RDR_Escape     APDU
  Len.....           ClInP1P2Lc
          Slot=0     pseudo-APDU DirectTransmit
            Seq=0a
              RFU=000000
8308000000000a028100            d50332010407  9000
RDR_to_PC_Escape                              SW: OK
  Len.....
          Slot=0
            Seq=0a
              Slot Status=02  ??
                Slot Error=81 ??
                  RFU=00


--------------------------------------------------------------------
Touchatag (Acr122U SAM) pseudo-APDU mechanism:
6f07000000000e000000 ff00000002 d402
PC_to_RDR_XfrBlock   APDU
  Len.....           ClInP1P2Lc
          Slot=0     pseudo-APDU DirectTransmit
            Seq=0e
              BWI=00
                RFU=0000
8002000000000e000000                          6108
RDR_to_PC_DataBlock                           SW: more data: 8 bytes
          Slot=0
            Seq=0e
              Slot Status=00
                Slot Error=00
                  RFU=00
6f05000000000f000000 ffc0000008
                     pseudo-ADPU GetResponse
8008000000000f000000            d50332010407  9000
                                              SW: OK
*/
// FIXME ACR122_USB_BUFFER_LEN don't have the correct lenght value
#define ACR122_USB_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
static int
acr122_build_frame_from_apdu(uint8_t **frame, const uint8_t *apdu, const size_t apdu_len)
{
  static uint8_t  abtFrame[ACR122_USB_BUFFER_LEN] = {
    0x6b, // PC_to_RDR_Escape
    0x00, // len
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // padding
  };
  if ((apdu_len + 10) > ACR122_USB_BUFFER_LEN)
    return NFC_EINVARG;

  abtFrame[1] = apdu_len;
  memcpy(abtFrame + 10, apdu, apdu_len);
  *frame = abtFrame;
  return (apdu_len + 10);
}

static int
acr122_build_frame_from_tama(uint8_t **frame, const uint8_t *tama, const size_t tama_len)
{
  static uint8_t  abtFrame[ACR122_USB_BUFFER_LEN] = {
    0x6b, // PC_to_RDR_Escape
    0x00, // len
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // padding
    // APDU
    0xff, 0x00, 0x00, 0x00,
    0x00, // PN532 command length
    0xd4, // PN532 direction
  };
  if ((tama_len + 16) > ACR122_USB_BUFFER_LEN)
    return NFC_EINVARG;

  abtFrame[1] = tama_len + 6;
  abtFrame[14] = tama_len + 1;
  memcpy(abtFrame + 16, tama, tama_len);
  *frame = abtFrame;
  return (tama_len + 16);
}

int
acr122_usb_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, const int timeout)
{
  uint8_t *frame;
  int res;
  if ((res = acr122_build_frame_from_tama(&frame, pbtData, szData)) < 0) {
    pnd->last_error = NFC_EINVARG;
    return pnd->last_error;
  }

  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), frame, res, timeout)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

#define USB_TIMEOUT_PER_PASS 200
int
acr122_usb_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, const int timeout)
{
  off_t offset = 0;

  uint8_t  abtRxBuf[ACR122_USB_BUFFER_LEN];
  int res;

  /*
   * If no timeout is specified but the command is blocking, force a 200ms (USB_TIMEOUT_PER_PASS)
   * timeout to allow breaking the loop if the user wants to stop it.
   */
  int usb_timeout;
  int remaining_time = timeout;
read:
  if (timeout == USB_INFINITE_TIMEOUT) {
    usb_timeout = USB_TIMEOUT_PER_PASS;
  } else {
    // A user-provided timeout is set, we have to cut it in multiple chunk to be able to keep an nfc_abort_command() mecanism
    remaining_time -= USB_TIMEOUT_PER_PASS;
    if (remaining_time <= 0) {
      pnd->last_error = NFC_ETIMEOUT;
      return pnd->last_error;
    } else {
      usb_timeout = MIN(remaining_time, USB_TIMEOUT_PER_PASS);
    }
  }

  res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), usb_timeout);

  if (res == NFC_ETIMEOUT) {
    if (DRIVER_DATA(pnd)->abort_flag) {
      DRIVER_DATA(pnd)->abort_flag = false;
      acr122_usb_ack(pnd);
      pnd->last_error = NFC_EOPABORTED;
      return pnd->last_error;
    } else {
      goto read;
    }
  }

  if (res < 0) {
    // try to interrupt current device state
    acr122_usb_ack(pnd);
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (abtRxBuf[offset] != 0x83) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Frame header mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset++;

  size_t len = abtRxBuf[offset++];
  if (len < 4) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Too small reply");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  len -= 4;

  if (len > szDataLen) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %zu, len: %zu)", szDataLen, len);
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }

  const uint8_t acr122_preamble[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x81, 0x00 };
  if (0 != (memcmp(abtRxBuf + offset, acr122_preamble, sizeof(acr122_preamble)))) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Frame preamble mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += sizeof(acr122_preamble);

  // TFI + PD0 (CC+1)
  if (abtRxBuf[offset] != 0xD5) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += 1;

  if (abtRxBuf[offset] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += 1;

  memcpy(pbtData, abtRxBuf + offset, len);
  offset += len;

  return len;
}

int
acr122_usb_ack(nfc_device *pnd)
{
  (void) pnd;
  int res = 0;
  uint8_t acr122_ack_frame[] = { GetFirmwareVersion }; // We can't send a PN532's ACK frame, so we use a normal command to cancel current command
  log_put(LOG_CATEGORY, NFC_PRIORITY_DEBUG, "%s", "ACR122 Abort");
  uint8_t *frame;
  if ((res = acr122_build_frame_from_tama(&frame, acr122_ack_frame, sizeof(acr122_ack_frame))) < 0)
    return res;

  res = acr122_usb_bulk_write(DRIVER_DATA(pnd), frame, res, 1000);
  uint8_t  abtRxBuf[ACR122_USB_BUFFER_LEN];
  res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), 1000);
  return res;
}

int
acr122_usb_init(nfc_device *pnd)
{
  int res = 0;
  uint8_t  abtRxBuf[ACR122_USB_BUFFER_LEN];

  /*
  // See ACR122 manual: "Bi-Color LED and Buzzer Control" section
  uint8_t acr122u_get_led_state_frame[] = {
    0x6b, // CCID
    0x09, // lenght of frame
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // padding
    // frame:
    0xff, // Class
    0x00, // INS
    0x40, // P1: Get LED state command
    0x00, // P2: LED state control
    0x04, // Lc
    0x00, 0x00, 0x00, 0x00, // Blinking duration control
  };

  log_put (LOG_CATEGORY, NFC_PRIORITY_DEBUG, "%s", "ACR122 Get LED state");
  if ((res = acr122_usb_bulk_write (DRIVER_DATA (pnd), (uint8_t *) acr122u_get_led_state_frame, sizeof (acr122u_get_led_state_frame), 1000)) < 0)
    return res;

  if ((res = acr122_usb_bulk_read (DRIVER_DATA (pnd), abtRxBuf, sizeof (abtRxBuf), 1000)) < 0)
    return res;
  */

  if ((res = pn53x_set_property_int(pnd, NP_TIMEOUT_COMMAND, 1000)) < 0)
    return res;

  uint8_t acr122u_set_picc_operating_parameters_off_frame[] = {
    0xff, // Class
    0x00, // INS
    0x51, // P1: Set PICC Operating Parameters
    0x00, // P2: New PICC Operating Parameters
    0x00, // Le
  };
  uint8_t *frame;

  log_put(LOG_CATEGORY, NFC_PRIORITY_DEBUG, "%s", "ACR122 PICC Operating Parameters");
  if ((res = acr122_build_frame_from_apdu(&frame, acr122u_set_picc_operating_parameters_off_frame, sizeof(acr122u_set_picc_operating_parameters_off_frame))) < 0)
    return res;
  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), frame, res, 1000)) < 0)
    return res;
  if ((res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), 1000)) < 0)
    return res;

  if ((res = pn53x_init(pnd)) < 0)
    return res;

  return NFC_SUCCESS;
}

static int
acr122_usb_abort_command(nfc_device *pnd)
{
  DRIVER_DATA(pnd)->abort_flag = true;
  return NFC_SUCCESS;
}

const struct pn53x_io acr122_usb_io = {
  .send       = acr122_usb_send,
  .receive    = acr122_usb_receive,
};

const struct nfc_driver acr122_usb_driver = {
  .name                             = ACR122_USB_DRIVER_NAME,
  .probe                            = acr122_usb_probe,
  .open                             = acr122_usb_open,
  .close                            = acr122_usb_close,
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

  .abort_command  = acr122_usb_abort_command,
  .idle  = pn53x_idle,
};
