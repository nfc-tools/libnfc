/*
 * Prototypes, structure definitions and macros.
 *
 * Copyright (c) 2000-2003 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * This file (and only this file) may alternatively be licensed under the
 * BSD license. See the LICENSE file shipped with the libusb-compat-0.1 source
 * distribution for details.
 */

#ifndef __USB_H__
#define __USB_H__

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#include <dirent.h>

/* Some quick and generic macros for the simple kind of lists we use */
#define LIST_ADD(begin, ent) \
  do { \
    if (begin) { \
      ent->next = begin; \
      ent->next->prev = ent; \
    } else \
      ent->next = NULL; \
    ent->prev = NULL; \
    begin = ent; \
  } while(0)

#define LIST_DEL(begin, ent) \
  do { \
    if (ent->prev) \
      ent->prev->next = ent->next; \
    else \
      begin = ent->next; \
    if (ent->next) \
      ent->next->prev = ent->prev; \
    ent->prev = NULL; \
    ent->next = NULL; \
  } while (0)

struct usbbus_dev_handle {
  libusb_device_handle *handle;
  struct usbbus_device *device;

  /* libusb-0.1 is buggy w.r.t. interface claiming. it allows you to claim
   * multiple interfaces but only tracks the most recently claimed one,
   * which is used for usb_set_altinterface(). we clone the buggy behaviour
   * here. */
  int last_claimed_interface;
};


/*
 * USB spec information
 *
 * This is all stuff grabbed from various USB specs and is pretty much
 * not subject to change
 */

#define USBBUS_DT_CONFIG_SIZE 9
#define USBBUS_DT_INTERFACE_SIZE 9
#define USBBUS_DT_ENDPOINT_AUDIO_SIZE 9

/* Variables */
extern struct usbbus_bus *usb_busses;

/* Function prototypes */

char *usb_strerror(void);

void usb_init(void);
int usb_find_busses(void);
int usb_find_devices(void);

#endif /* __USB_H__ */

