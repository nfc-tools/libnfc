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
 *
 */

/**
 * @file usbbus.h
 * @brief libusb 0.1 driver header
 */

#ifndef __NFC_BUS_USB_H__
#  define __NFC_BUS_USB_H__

#include <stdbool.h>
#include <string.h>

#define USBBUS_ERROR_ACCESS  -3
#define USBBUS_ERROR_TIMEOUT -7

int usbbus_prepare(void);

// Libusb-0.1 API:
#define USBBUS_ENDPOINT_DIR_MASK           0x80
#define USBBUS_ENDPOINT_TYPE_BULK          2
#define USBBUS_ENDPOINT_IN                 0x80
#define USBBUS_ENDPOINT_OUT                0x00

#ifdef PATH_MAX
#define USBBUS_PATH_MAX PATH_MAX
#else
#define USBBUS_PATH_MAX 4096
#endif

struct usbbus_device_handle;
typedef struct usbbus_device_handle usbbus_device_handle;

struct usbbus_endpoint_descriptor {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bEndpointAddress;
  uint8_t  bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t  bInterval;
  uint8_t  bRefresh;
  uint8_t  bSynchAddress;

  unsigned char *extra;
  int extralen;
};

struct usbbus_interface_descriptor {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bInterfaceNumber;
  uint8_t  bAlternateSetting;
  uint8_t  bNumEndpoints;
  uint8_t  bInterfaceClass;
  uint8_t  bInterfaceSubClass;
  uint8_t  bInterfaceProtocol;
  uint8_t  iInterface;

  struct usbbus_endpoint_descriptor *endpoint;

  unsigned char *extra;
  int extralen;
};

struct usbbus_interface {
  struct usbbus_interface_descriptor *altsetting;
  int num_altsetting;
};

struct usbbus_config_descriptor {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t wTotalLength;
  uint8_t  bNumInterfaces;
  uint8_t  bConfigurationValue;
  uint8_t  iConfiguration;
  uint8_t  bmAttributes;
  uint8_t  MaxPower;

  struct usbbus_interface *interface;

  unsigned char *extra;
  int extralen;
};

/* Device descriptor */
struct usbbus_device_descriptor {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t bcdUSB;
  uint8_t  bDeviceClass;
  uint8_t  bDeviceSubClass;
  uint8_t  bDeviceProtocol;
  uint8_t  bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t  iManufacturer;
  uint8_t  iProduct;
  uint8_t  iSerialNumber;
  uint8_t  bNumConfigurations;
};


struct usbbus_bus;

struct usbbus_device {
  struct usbbus_device *next, *prev;

  char filename[USBBUS_PATH_MAX + 1];

  struct usbbus_bus *bus;

  struct usbbus_device_descriptor descriptor;
  struct usbbus_config_descriptor *config;

  void *dev;

  uint8_t devnum;

  unsigned char num_children;
  struct usbbus_device **children;
};

struct usbbus_bus {
  struct usbbus_bus *next, *prev;

  char dirname[USBBUS_PATH_MAX + 1];

  struct usbbus_device *devices;
  uint32_t location;

  struct usbbus_device *root_dev;
};


usbbus_device_handle *usbbus_open(struct usbbus_device *dev);
void usbbus_close(usbbus_device_handle *dev);
int usbbus_set_configuration(usbbus_device_handle *dev, int configuration);
int usbbus_get_string_simple(usbbus_device_handle *dev, int index, char *buf, size_t buflen);
int usbbus_bulk_transfer(usbbus_device_handle *dev, int ep, char *bytes, int size, int *actual_length, int timeout);
int usbbus_claim_interface(usbbus_device_handle *dev, int interface);
int usbbus_release_interface(usbbus_device_handle *dev, int interface);
int usbbus_set_interface_alt_setting(usbbus_device_handle *dev, int interface, int alternate);
int usbbus_reset(usbbus_device_handle *dev);
const char *usbbus_strerror(int errcode);
struct usbbus_bus *usbbus_get_busses(void);

#endif // __NFC_BUS_USB_H__
