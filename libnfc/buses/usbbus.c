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
#include <errno.h>
#include <libusb.h>
#include "libusb-compat-usb.h"
#include "usbbus.h"
#include "log.h"
#define LOG_CATEGORY "libnfc.buses.usbbus"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

#define compat_err(e) -(errno=libusb_to_errno(e))

static int libusb_to_errno(int result)
{
  switch (result) {
    case LIBUSB_SUCCESS:
      return 0;
    case LIBUSB_ERROR_IO:
      return EIO;
    case LIBUSB_ERROR_INVALID_PARAM:
      return EINVAL;
    case LIBUSB_ERROR_ACCESS:
      return EACCES;
    case LIBUSB_ERROR_NO_DEVICE:
      return ENXIO;
    case LIBUSB_ERROR_NOT_FOUND:
      return ENOENT;
    case LIBUSB_ERROR_BUSY:
      return EBUSY;
    case LIBUSB_ERROR_TIMEOUT:
      return ETIMEDOUT;
    case LIBUSB_ERROR_OVERFLOW:
      return EOVERFLOW;
    case LIBUSB_ERROR_PIPE:
      return EPIPE;
    case LIBUSB_ERROR_INTERRUPTED:
      return EINTR;
    case LIBUSB_ERROR_NO_MEM:
      return ENOMEM;
    case LIBUSB_ERROR_NOT_SUPPORTED:
      return ENOSYS;
    default:
      return ERANGE;
  }
}


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
  int r;
  usbbus_dev_handle *udev = malloc(sizeof(*udev));
  if (!udev)
    return NULL;

  r = libusb_open((libusb_device *) dev->dev, &udev->handle);
  if (r < 0) {
    free(udev);
    errno = libusb_to_errno(r);
    return NULL;
  }

  udev->last_claimed_interface = -1;
  udev->device = dev;
  return udev;
}

int usbbus_close(usbbus_dev_handle *dev)
{
  libusb_close(dev->handle);
  free(dev);
  return 0;
}

int usbbus_set_configuration(usbbus_dev_handle *dev, int configuration)
{
  return compat_err(libusb_set_configuration(dev->handle, configuration));
}

int usbbus_get_string_simple(usbbus_dev_handle *dev, int index, char *buf, size_t buflen)
{
  int r;
  r = libusb_get_string_descriptor_ascii(dev->handle, index & 0xff,
                                         (unsigned char *) buf, (int) buflen);
  if (r >= 0)
    return r;
  return compat_err(r);
}

static int usbbus_bulk_io(usbbus_dev_handle *dev, int ep, unsigned char *bytes,
                       int size, int timeout)
{
  int actual_length;
  int r;
  r = libusb_bulk_transfer(dev->handle, ep & 0xff, bytes, size,
                           &actual_length, timeout);

  /* if we timed out but did transfer some data, report as successful short
   * read. FIXME: is this how libusb-0.1 works? */
  if (r == 0 || (r == LIBUSB_ERROR_TIMEOUT && actual_length > 0))
    return actual_length;

  return compat_err(r);
}

int usbbus_bulk_read(usbbus_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
  return usbbus_bulk_io(dev, ep, (unsigned char *) bytes, size, timeout);
}

int usbbus_bulk_write(usbbus_dev_handle *dev, int ep, const char *bytes, int size, int timeout)
{
  return usbbus_bulk_io(dev, ep, (unsigned char *)bytes, size, timeout);
}

int usbbus_claim_interface(usbbus_dev_handle *dev, int interface)
{
  int r;
  r = libusb_claim_interface(dev->handle, interface);
  if (r == 0) {
    dev->last_claimed_interface = interface;
    return 0;
  }
  return compat_err(r);
}

int usbbus_release_interface(usbbus_dev_handle *dev, int interface)
{
  int r;
  r = libusb_release_interface(dev->handle, interface);
  if (r == 0)
    dev->last_claimed_interface = -1;
  return compat_err(r);
}

int usbbus_set_altinterface(usbbus_dev_handle *dev, int alternate)
{
  if (dev->last_claimed_interface < 0)
    return -(errno=EINVAL);
  return compat_err(libusb_set_interface_alt_setting(dev->handle, dev->last_claimed_interface, alternate));
}

int usbbus_reset(usbbus_dev_handle *dev)
{
  return compat_err(libusb_reset_device(dev->handle));
}

struct usbbus_bus *usbbus_get_busses(void)
{
  return usb_busses;
}

