/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Copyright (C) 2022      Kenspeckle
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

#ifndef __NFC_BUS_USBBUS_H__
#define __NFC_BUS_USBBUS_H__

#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include "nfc/nfc-types.h"

#define EMPTY_STRING ((unsigned char *)"\0")

struct usbbus_device {
  uint16_t vendor_id;
  uint16_t product_id;
  const char *name;
  uint16_t max_packet_size;
};



int usbbus_prepare();

size_t usbbus_usb_scan(nfc_connstring connstrings[], size_t connstrings_len, struct usbbus_device * nfc_usb_devices, size_t num_nfc_usb_devices, char * usb_driver_name);
void usbbus_get_usb_endpoints(struct libusb_device *dev, uint8_t * endpoint_in, uint8_t * endpoint_out, uint16_t * max_packet_size);
void usbbus_get_usb_device_name(struct libusb_device * dev, libusb_device_handle *udev, char *buffer, size_t len);
void usbbus_get_device(uint8_t dev_address, struct libusb_device ** dev, struct libusb_device_handle ** dev_handle);
void usbbus_close(struct libusb_device * dev, struct libusb_device_handle * dev_handle);
uint16_t usbbus_get_vendor_id(struct libusb_device * dev);
uint16_t usbbus_get_product_id(struct libusb_device * dev);
int usbbus_get_num_alternate_settings(struct libusb_device *dev, uint8_t config_idx);
#endif
