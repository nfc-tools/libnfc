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
 * @file usbbus.c
 * @brief libusb 0.1 driver wrapper
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdlib.h>

#ifndef _WIN32
// #ifdef LIBUSB10_ENABLED
 #include "libusb-compat-usb.h"
// #else
 // Under POSIX system, we use libusb (>= 0.1.12)
// #include <usb.h>
// #endif
#else
// Under Windows we use libusb-win32 (>= 1.2.5)
#include <lusb0_usb.h>
#endif

#include "usbbus.h"
#include "log.h"
#define LOG_CATEGORY "libnfc.buses.usbbus"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

int usbbus_prepare(void)
{
  static bool usb_initialized = false;
  if (!usb_initialized) {

#ifdef ENVVARS
    char *env_log_level = getenv("LIBNFC_LOG_LEVEL");
    // Set libusb debug only if asked explicitely:
    // LIBUSB_LOG_LEVEL=12288 (= NFC_LOG_PRIORITY_DEBUG * 2 ^ NFC_LOG_GROUP_LIBUSB)
    if (env_log_level && (((atoi(env_log_level) >> (NFC_LOG_GROUP_LIBUSB * 2)) & 0x00000003) >= NFC_LOG_PRIORITY_DEBUG)) {
      setenv("USB_DEBUG", "255", 1);
    }
#endif

    usb_init();
    usb_initialized = true;
  }

  int res;
  // usb_find_busses will find all of the busses on the system. Returns the
  // number of changes since previous call to this function (total of new
  // busses and busses removed).
  if ((res = usb_find_busses()) < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to find USB busses (%s)", usbbus_strerror(res));
    return -1;
  }
  // usb_find_devices will find all of the devices on each bus. This should be
  // called after usb_find_busses. Returns the number of changes since the
  // previous call to this function (total of new device and devices removed).
  if ((res = usb_find_devices()) < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to find USB devices (%s)", usbbus_strerror(res));
    return -1;
  }
  return 0;
}

usbbus_dev_handle *usbbus_open(struct usbbus_device *dev)
{
  return (usbbus_dev_handle *) usb_open((struct usb_device *) dev);
}

int usbbus_close(usbbus_dev_handle *dev)
{
  return usb_close((usb_dev_handle *) dev);
}

int usbbus_set_configuration(usbbus_dev_handle *dev, int configuration)
{
  return usb_set_configuration((usb_dev_handle *)dev, configuration);
}

int usbbus_get_string_simple(usbbus_dev_handle *dev, int index, char *buf, size_t buflen)
{
  return usb_get_string_simple((usb_dev_handle *)dev, index, buf, buflen);
}

int usbbus_bulk_read(usbbus_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
  return usb_bulk_read((usb_dev_handle *)dev, ep, bytes, size, timeout);
}

int usbbus_bulk_write(usbbus_dev_handle *dev, int ep, const char *bytes, int size, int timeout)
{
  return usb_bulk_write((usb_dev_handle *)dev, ep, bytes, size, timeout);
}

int usbbus_claim_interface(usbbus_dev_handle *dev, int interface)
{
  return usb_claim_interface((usb_dev_handle *)dev, interface);
}

int usbbus_release_interface(usbbus_dev_handle *dev, int interface)
{
  return usb_release_interface((usb_dev_handle *)dev, interface);
}

int usbbus_set_altinterface(usbbus_dev_handle *dev, int alternate)
{
  return usb_set_altinterface((usb_dev_handle *)dev, alternate);
}

int usbbus_reset(usbbus_dev_handle *dev)
{
  return usb_reset((usb_dev_handle *)dev);
}

struct usbbus_device *usbbus_device(usbbus_dev_handle *dev)
{
  return (struct usbbus_device *) usb_device((usb_dev_handle *)dev);
}

struct usbbus_bus *usbbus_get_busses(void)
{
  return (struct usbbus_bus *) usb_get_busses();
}

