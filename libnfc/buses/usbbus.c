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
 * @brief libusb 1.0 driver wrapper
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdlib.h>

#include "usbbus.h"
#include "log.h"
#define LOG_CATEGORY "libnfc.buses.usbbus"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

static libusb_context *usb_context = NULL;

libusb_context *
usb_get_context(void)
{
  static bool usb_initialized = false;
  if (!usb_initialized) {
    int r = libusb_init(&usb_context);
    if (r < 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to initialize libusb (%s)", libusb_strerror(r));
      return NULL;
    }

#ifdef ENVVARS
    char *env_log_level = getenv("LIBNFC_LOG_LEVEL");
    // Set libusb debug only if asked explicitly:
    // LIBUSB_LOG_LEVEL=12288 (= NFC_LOG_PRIORITY_DEBUG * 2 ^ NFC_LOG_GROUP_LIBUSB)
    if (env_log_level && (((atoi(env_log_level) >> (NFC_LOG_GROUP_LIBUSB * 2)) & 0x00000003) >= NFC_LOG_PRIORITY_DEBUG)) {
      libusb_set_option(usb_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    }
#endif

    usb_initialized = true;
  }

  return usb_context;
}

