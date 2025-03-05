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
#include <string.h>
#ifdef _MSC_VER
#include <sys/types.h>
#endif
#include <nfc/nfc.h>

#include "nfc-internal.h"
#include "buses/usbbus.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "drivers/acr122_usb.h"

#define ACR122_USB_DRIVER_NAME "acr122_usb"

#define LOG_GROUP     NFC_LOG_GROUP_DRIVER
#define LOG_CATEGORY "libnfc.driver.acr122_usb"

#define USBBUS_INFINITE_TIMEOUT   0

#define DRIVER_DATA(pnd) ((struct acr122_usb_data*)(pnd->driver_data))

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

--------------------------------------------------------------------
Apparently Acr122U PICC can also work without Escape (even if there is no card):
6f070000000000000000 ff00000002 d402
PC_to_RDR_XfrBlock   APDU
  Len.....           ClInP1P2Lc
          Slot=0     pseudo-APDU DirectTransmit
            Seq=00
              BWI=00
                RFU=0000
80080000000000008100            d50332010407  9000
                                              SW: OK
*/

#pragma pack(1)
struct ccid_header {
  uint8_t bMessageType;
  uint32_t dwLength;
  uint8_t bSlot;
  uint8_t bSeq;
  uint8_t bMessageSpecific[3];
};

struct apdu_header {
  uint8_t bClass;
  uint8_t bIns;
  uint8_t bP1;
  uint8_t bP2;
  uint8_t bLen;
};

struct acr122_usb_tama_frame {
  struct ccid_header ccid_header;
  struct apdu_header apdu_header;
  uint8_t tama_header;
  uint8_t tama_payload[254]; // According to ACR122U manual: Pseudo APDUs (Section 6.0), Lc is 1-byte long (Data In: 255-bytes).
};

struct acr122_usb_apdu_frame {
  struct ccid_header ccid_header;
  struct apdu_header apdu_header;
  uint8_t apdu_payload[255]; // APDU Lc is 1-byte long
};
#pragma pack()

// Internal data struct
struct acr122_usb_data {
  libusb_device * dev;
  libusb_device_handle *pudh;
  uint8_t configIdx;
  uint8_t uiEndPointIn;
  uint8_t uiEndPointOut;
  uint16_t uiMaxPacketSize;
  volatile bool abort_flag;
  // Keep some buffers to reduce memcpy() usage
  struct acr122_usb_tama_frame tama_frame;
  struct acr122_usb_apdu_frame apdu_frame;
};

// CCID Bulk-Out messages type
#define PC_to_RDR_IccPowerOn	0x62
#define PC_to_RDR_XfrBlock	0x6f

#define RDR_to_PC_DataBlock	0x80

// ISO 7816-4
#define SW1_More_Data_Available 0x61
#define SW1_Warning_with_NV_changed 0x63
#define PN53x_Specific_Application_Level_Error_Code 0x7f


// This frame template is copied at init time
// Its designed for TAMA sending but is also used for simple ADPU frame: acr122_build_frame_from_apdu() will overwrite needed bytes
const uint8_t acr122_usb_frame_template[] = {
  PC_to_RDR_XfrBlock, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CCID header
  0xff, 0x00, 0x00, 0x00, 0x00, // ADPU header
  0xd4, // PN532 direction
};

// APDUs instructions
#define APDU_GetAdditionnalData 0xc0

// Internal io struct
const struct pn53x_io acr122_usb_io;

// Prototypes
static int acr122_usb_init(nfc_device *pnd);
static int acr122_usb_ack(nfc_device *pnd);
static int acr122_usb_send_apdu(nfc_device *pnd,
                                const uint8_t ins, const uint8_t p1, const uint8_t p2, const uint8_t *const data, size_t data_len, const uint8_t le,
                                uint8_t *out, const size_t out_size);

static int
acr122_usb_bulk_read(struct acr122_usb_data *data, uint8_t abtRx[], const size_t szRx, const int timeout)
{
  int actual_length;
  int res = libusb_bulk_transfer(data->pudh, data->uiEndPointIn, (unsigned char *) abtRx, szRx, &actual_length, timeout);
  if (res == 0) {
    LOG_HEX(NFC_LOG_GROUP_COM, "RX", abtRx, actual_length);
    res = actual_length;
  } else {
    if (res != LIBUSB_ERROR_TIMEOUT) {
      res = NFC_EIO;
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to read from USB (%s)", libusb_strerror(res));
    } else {
      res = NFC_ETIMEOUT;
    }
  }
  return res;
}

static int
acr122_usb_bulk_write(struct acr122_usb_data *data, uint8_t abtTx[], const size_t szTx, const int timeout)
{
  LOG_HEX(NFC_LOG_GROUP_COM, "TX", abtTx, szTx);
  int actual_length;
  int res = libusb_bulk_transfer(data->pudh, data->uiEndPointOut, (unsigned char *) abtTx, szTx, &actual_length, timeout);
  if (res == 0) {
    // HACK This little hack is a well know problem of USB, see http://www.libusb.org/ticket/6 for more details
    if ((actual_length > 0) && ((actual_length % data->uiMaxPacketSize) == 0)) {
      libusb_bulk_transfer(data->pudh, data->uiEndPointOut, EMPTY_STRING, 0, &actual_length, timeout);
    }
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to write to USB (%s)", libusb_strerror(res));
    if (res == LIBUSB_ERROR_TIMEOUT) {
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
  const char *name;
  uint16_t max_packet_size;
};

const struct acr122_usb_supported_device acr122_usb_supported_devices[] = {
  { 0x072F, 0x2200, "ACS ACR122", 0x40 },
  { 0x072F, 0x90CC, "Touchatag", 0x40 },
  { 0x072F, 0x2214, "ACS ACR1222", 0x40 },
};

const size_t num_acr122_usb_supported_device = sizeof(acr122_usb_supported_devices) / sizeof(struct acr122_usb_supported_device);



static size_t
acr122_usb_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  (void) context;
  struct usbbus_device devices[num_acr122_usb_supported_device];
  for (size_t i = 0; i < num_acr122_usb_supported_device; i++) {
    devices[i].product_id = acr122_usb_supported_devices[i].product_id;
    devices[i].vendor_id = acr122_usb_supported_devices[i].vendor_id;
    devices[i].name = acr122_usb_supported_devices[i].name;
    devices[i].max_packet_size = acr122_usb_supported_devices[i].max_packet_size;
  }
  return usbbus_usb_scan(connstrings, connstrings_len, devices, num_acr122_usb_supported_device, ACR122_USB_DRIVER_NAME);
}

static bool
acr122_usb_get_usb_device_name(struct libusb_device *dev, libusb_device_handle *udev, char *buffer, size_t len)
{
  *buffer = '\0';

  usbbus_get_usb_device_name(dev, udev, buffer, len);
  uint16_t vendor_id = usbbus_get_vendor_id(dev);
  uint16_t product_id = usbbus_get_product_id(dev);

  if (!*buffer) {
    for (size_t n = 0; n < num_acr122_usb_supported_device; n++) {
      if ((acr122_usb_supported_devices[n].vendor_id == vendor_id) &&
          (acr122_usb_supported_devices[n].product_id == product_id)) {
        strncpy(buffer, acr122_usb_supported_devices[n].name, len);
        buffer[len - 1] = '\0';
        return true;
      }
    }
  }

  return false;
}

static nfc_device *
acr122_usb_open(const nfc_context *context, const nfc_connstring connstring)
{
  nfc_device *pnd = NULL;
  char *dev_address_str;
  char *config_idx_str;
  int connstring_decode_level = connstring_decode(connstring, ACR122_USB_DRIVER_NAME, "usb", &dev_address_str, &config_idx_str);
  log_put(LOG_GROUP,
          LOG_CATEGORY,
          NFC_LOG_PRIORITY_DEBUG,
          "%d element(s) have been decoded from \"%s\"",
          connstring_decode_level,
          connstring);
  if (connstring_decode_level < 2) {
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }

  uint8_t dev_addres = atoi(dev_address_str);
  uint8_t config_idx = atoi(config_idx_str);

  usbbus_prepare();
  struct acr122_usb_data data = {
    .dev = NULL,
    .pudh = NULL,
    .configIdx = config_idx,
    .uiEndPointIn = 0,
    .uiEndPointOut = 0,
  };
  usbbus_get_device(dev_addres, &data.dev, &data.pudh);
  // Reset device
  libusb_reset_device(data.pudh);

  // Retrieve end points
  usbbus_get_usb_endpoints(data.dev, &(data.uiEndPointIn), &(data.uiEndPointOut), &(data.uiMaxPacketSize));
  // Claim interface
  int res = libusb_claim_interface(data.pudh, 0);
  if (res < 0) {
    log_put(LOG_GROUP,
            LOG_CATEGORY,
            NFC_LOG_PRIORITY_ERROR,
            "Unable to claim USB interface (%s)",
            libusb_strerror(res));
    libusb_close(data.pudh);
    // we failed to use the specified device
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }

  // Check if there are more than 0 alternative interfaces and claim the first one
  if (usbbus_get_num_alternate_settings(data.dev, data.configIdx) > 0) {
    res = libusb_set_interface_alt_setting(data.pudh, 0, 0);
    if (res < 0) {
      log_put(LOG_GROUP,
              LOG_CATEGORY,
              NFC_LOG_PRIORITY_ERROR,
              "Unable to set alternate setting on USB interface (%s)",
              libusb_strerror(res));
      libusb_close(data.pudh);
      // we failed to use the specified device
      free(dev_address_str);
      free(config_idx_str);
      return NULL;
    }
  }

  // Allocate memory for the device info and specification, fill it and return the info
  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }
  acr122_usb_get_usb_device_name(data.dev, data.pudh, pnd->name, sizeof(pnd->name));

  pnd->driver_data = malloc(sizeof(struct acr122_usb_data));
  if (!pnd->driver_data) {
    perror("malloc");
    nfc_device_free(pnd);
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }
  *DRIVER_DATA(pnd) = data;

  // Alloc and init chip's data
  if (pn53x_data_new(pnd, &acr122_usb_io) == NULL) {
    perror("malloc");
    nfc_device_free(pnd);
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }

  memcpy(&(DRIVER_DATA(pnd)->tama_frame), acr122_usb_frame_template, sizeof(acr122_usb_frame_template));
  memcpy(&(DRIVER_DATA(pnd)->apdu_frame), acr122_usb_frame_template, sizeof(acr122_usb_frame_template));
  CHIP_DATA(pnd)->timer_correction = 46; // empirical tuning
  pnd->driver = &acr122_usb_driver;

  if (acr122_usb_init(pnd) < 0) {
    libusb_close(data.pudh);
    nfc_device_free(pnd);
    free(dev_address_str);
    free(config_idx_str);
    return NULL;
  }
  DRIVER_DATA(pnd)->abort_flag = false;

  free(dev_address_str);
  free(config_idx_str);
  return pnd;
}

static void
acr122_usb_close(nfc_device *pnd)
{
  acr122_usb_ack(pnd);
  pn53x_idle(pnd);

  int res = libusb_release_interface(DRIVER_DATA(pnd)->pudh, 0);
  if (res < 0) {
    log_put(LOG_GROUP,
            LOG_CATEGORY,
            NFC_LOG_PRIORITY_ERROR,
            "Unable to release USB interface (%s)",
            libusb_strerror(res));
  }

  usbbus_close(DRIVER_DATA(pnd)->dev, DRIVER_DATA(pnd)->pudh);

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

#if !defined(htole32)

uint32_t htole32(uint32_t u32);

uint32_t
htole32(uint32_t u32)
{
  union {
    uint8_t arr[4];
    uint32_t u32;
  } u;
  for (int i = 0; i < 4; i++) {
    u.arr[i] = (u32 & 0xff);
    u32 >>= 8;
  }
  return u.u32;
}

#endif /* !defined(htole32) */

static int
acr122_build_frame_from_apdu(nfc_device *pnd, const uint8_t ins, const uint8_t p1, const uint8_t p2, const uint8_t *data, const size_t data_len, const uint8_t le)
{
  if (data_len > sizeof(DRIVER_DATA(pnd)->apdu_frame.apdu_payload))
    return NFC_EINVARG;
  if ((data == NULL) && (data_len != 0))
    return NFC_EINVARG;

  DRIVER_DATA(pnd)->apdu_frame.ccid_header.dwLength = htole32(data_len + sizeof(struct apdu_header));
  DRIVER_DATA(pnd)->apdu_frame.apdu_header.bIns = ins;
  DRIVER_DATA(pnd)->apdu_frame.apdu_header.bP1 = p1;
  DRIVER_DATA(pnd)->apdu_frame.apdu_header.bP2 = p2;
  if (data) {
    // bLen is Lc when data != NULL
    DRIVER_DATA(pnd)->apdu_frame.apdu_header.bLen = data_len;
    memcpy(DRIVER_DATA(pnd)->apdu_frame.apdu_payload, data, data_len);
  } else {
    // bLen is Le when no data.
    DRIVER_DATA(pnd)->apdu_frame.apdu_header.bLen = le;
  }
  return (sizeof(struct ccid_header) + sizeof(struct apdu_header) + data_len);
}

static int
acr122_build_frame_from_tama(nfc_device *pnd, const uint8_t *tama, const size_t tama_len)
{
  if (tama_len > sizeof(DRIVER_DATA(pnd)->tama_frame.tama_payload))
    return NFC_EINVARG;

  DRIVER_DATA(pnd)->tama_frame.ccid_header.dwLength = htole32(tama_len + sizeof(struct apdu_header) + 1);
  DRIVER_DATA(pnd)->tama_frame.apdu_header.bLen = tama_len + 1;
  memcpy(DRIVER_DATA(pnd)->tama_frame.tama_payload, tama, tama_len);
  return (sizeof(struct ccid_header) + sizeof(struct apdu_header) + 1 + tama_len);
}

static int
acr122_usb_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, const int timeout)
{
  int res;
  if ((res = acr122_build_frame_from_tama(pnd, pbtData, szData)) < 0) {
    pnd->last_error = NFC_EINVARG;
    return pnd->last_error;
  }

  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), (unsigned char *) & (DRIVER_DATA(pnd)->tama_frame), res, timeout)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

#define USBBUS_TIMEOUT_PER_PASS 200

static int
acr122_usb_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, const int timeout)
{
  off_t offset = 0;

  uint8_t  abtRxBuf[255 + sizeof(struct ccid_header)];
  int res;

  /*
   * If no timeout is specified but the command is blocking, force a 200ms (USBBUS_TIMEOUT_PER_PASS)
   * timeout to allow breaking the loop if the user wants to stop it.
   */
  int usbbus_timeout;
  int remaining_time = timeout;
read:
  if (timeout == USBBUS_INFINITE_TIMEOUT) {
    usbbus_timeout = USBBUS_TIMEOUT_PER_PASS;
  } else {
    // A user-provided timeout is set, we have to cut it in multiple chunk to be able to keep an nfc_abort_command() mechanism
    remaining_time -= USBBUS_TIMEOUT_PER_PASS;
    if (remaining_time <= 0) {
      pnd->last_error = NFC_ETIMEOUT;
      return pnd->last_error;
    } else {
      usbbus_timeout = MIN(remaining_time, USBBUS_TIMEOUT_PER_PASS);
    }
  }

  res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), usbbus_timeout);

  uint8_t attempted_response = RDR_to_PC_DataBlock;
  size_t len;
  int error, status;

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
  if (res < 10) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Invalid RDR_to_PC_DataBlock frame");
    // try to interrupt current device state
    acr122_usb_ack(pnd);
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  if (abtRxBuf[offset] != attempted_response) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame header mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset++;

  len = abtRxBuf[offset++];
  status = abtRxBuf[7];
  error = abtRxBuf[8];
  if (len == 0 && error == 0xFE) { // ICC_MUTE; XXX check for more errors
      // Do not check status; my ACR122U seemingly has status=0 in this case,
      // even though the spec says it should have had bmCommandStatus=1
      // and bmICCStatus=1.
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Command timed out");
      pnd->last_error = NFC_ETIMEOUT;
      return pnd->last_error;
  }

  if (!((len > 1) && (abtRxBuf[10] == 0xd5))) { // In case we didn't get an immediate answer:
    if (len != 2) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Wrong reply");
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
    if (abtRxBuf[10] != SW1_More_Data_Available) {
      if ((abtRxBuf[10] == SW1_Warning_with_NV_changed) && (abtRxBuf[11] == PN53x_Specific_Application_Level_Error_Code)) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "PN532 has detected an error at the application level");
      } else if ((abtRxBuf[10] == SW1_Warning_with_NV_changed) && (abtRxBuf[11] == 0x00)) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "PN532 didn't reply");
      } else {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unexpected Status Word (SW1: %02x SW2: %02x)", abtRxBuf[10], abtRxBuf[11]);
      }
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
    res = acr122_usb_send_apdu(pnd, APDU_GetAdditionnalData, 0x00, 0x00, NULL, 0, abtRxBuf[11], abtRxBuf, sizeof(abtRxBuf));
    if (res == NFC_ETIMEOUT) {
      if (DRIVER_DATA(pnd)->abort_flag) {
        DRIVER_DATA(pnd)->abort_flag = false;
        acr122_usb_ack(pnd);
        pnd->last_error = NFC_EOPABORTED;
        return pnd->last_error;
      } else {
        goto read; // FIXME May cause some trouble on Touchatag, right ?
      }
    }
    if (res < 10) {
      // try to interrupt current device state
      acr122_usb_ack(pnd);
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
  }
  offset = 0;
  if (abtRxBuf[offset] != attempted_response) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame header mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset++;

  // XXX In CCID specification, len is a 32-bits (dword), do we need to decode more than 1 byte ? (0-255 bytes for PN532 reply)
  len = abtRxBuf[offset++];
  if ((abtRxBuf[offset] != 0x00) && (abtRxBuf[offset + 1] != 0x00) && (abtRxBuf[offset + 2] != 0x00)) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Not implemented: only 1-byte length is supported, please report this bug with a full trace.");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += 3;

  if (len < 4) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Too small reply");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  len -= 4; // We skip 2 bytes for PN532 direction byte (D5) and command byte (CMD+1), then 2 bytes for APDU status (90 00).

  if (len > szDataLen) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %" PRIuPTR ", len: %" PRIuPTR ")", szDataLen, len);
    pnd->last_error = NFC_EOVFLOW;
    return pnd->last_error;
  }

  // Skip CCID remaining bytes
  offset += 2; // bSlot and bSeq are not used
  offset += 2; // bStatus and bError is partially checked
  offset += 1; // bRFU should be 0x00

  // TFI + PD0 (CC+1)
  if (abtRxBuf[offset] != 0xD5) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += 1;

  if (abtRxBuf[offset] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  offset += 1;

  memcpy(pbtData, abtRxBuf + offset, len);

  return len;
}

int
acr122_usb_ack(nfc_device *pnd)
{
  (void) pnd;
  int res = 0;
  uint8_t acr122_ack_frame[] = { GetFirmwareVersion }; // We can't send a PN532's ACK frame, so we use a normal command to cancel current command
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "ACR122 Abort");
  if ((res = acr122_build_frame_from_tama(pnd, acr122_ack_frame, sizeof(acr122_ack_frame))) < 0)
    return res;
  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), (unsigned char *) & (DRIVER_DATA(pnd)->tama_frame), res, 1000)) < 0)
    return res;
  uint8_t  abtRxBuf[255 + sizeof(struct ccid_header)];
  res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), 1000);
  return res;
}

static int
acr122_usb_send_apdu(nfc_device *pnd,
                     const uint8_t ins, const uint8_t p1, const uint8_t p2, const uint8_t *const data, size_t data_len, const uint8_t le,
                     uint8_t *out, const size_t out_size)
{
  int res;
  size_t frame_len = acr122_build_frame_from_apdu(pnd, ins, p1, p2, data, data_len, le);
  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), (unsigned char *) & (DRIVER_DATA(pnd)->apdu_frame), frame_len, 1000)) < 0)
    return res;
  if ((res = acr122_usb_bulk_read(DRIVER_DATA(pnd), out, out_size, 1000)) < 0)
    return res;
  return res;
}

int
acr122_usb_init(nfc_device *pnd)
{
  int res = 0;
  int i;
  uint8_t  abtRxBuf[255 + sizeof(struct ccid_header)];

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

  log_put (LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "ACR122 Get LED state");
  if ((res = acr122_usb_bulk_write (DRIVER_DATA (pnd), (uint8_t *) acr122u_get_led_state_frame, sizeof (acr122u_get_led_state_frame), 1000)) < 0)
    return res;

  if ((res = acr122_usb_bulk_read (DRIVER_DATA (pnd), abtRxBuf, sizeof (abtRxBuf), 1000)) < 0)
    return res;
  */

  if ((res = pn53x_set_property_int(pnd, NP_TIMEOUT_COMMAND, 1000)) < 0)
    return res;

  // Power On ICC
  uint8_t ccid_frame[] = {
    PC_to_RDR_IccPowerOn, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
  };

  if ((res = acr122_usb_bulk_write(DRIVER_DATA(pnd), ccid_frame, sizeof(struct ccid_header), 1000)) < 0)
    return res;
  if ((res = acr122_usb_bulk_read(DRIVER_DATA(pnd), abtRxBuf, sizeof(abtRxBuf), 1000)) < 0)
    return res;

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "ACR122 PICC Operating Parameters");
  if ((res = acr122_usb_send_apdu(pnd, 0x00, 0x51, 0x00, NULL, 0, 0, abtRxBuf, sizeof(abtRxBuf))) < 0)
    return res;

  res = 0;
  for (i = 0; i < 3; i++) {
    if (res < 0)
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "PN532 init failed, trying again...");
    if ((res = pn53x_init(pnd)) >= 0)
      break;
  }
  if (res < 0)
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
  .scan_type                        = NOT_INTRUSIVE,
  .scan                             = acr122_usb_scan,
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
  .idle           = pn53x_idle,
  /* Even if PN532, PowerDown is not recommended on those devices */
  .powerdown      = NULL,
};
