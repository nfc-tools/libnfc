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

#define USB_DT_CONFIG_SIZE 9
#define USB_DT_INTERFACE_SIZE 9
#define USB_DT_ENDPOINT_AUDIO_SIZE 9

/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
};

/* String descriptor */
struct usb_string_descriptor {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
  u_int16_t wData[1];
};

/* Endpoint descriptor */
#define USB_MAXENDPOINTS	32
struct usb_endpoint_descriptor {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
  u_int8_t  bEndpointAddress;
  u_int8_t  bmAttributes;
  u_int16_t wMaxPacketSize;
  u_int8_t  bInterval;
  u_int8_t  bRefresh;
  u_int8_t  bSynchAddress;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

#define USB_ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_TYPE_MASK		0x03    /* in bmAttributes */
#define USB_ENDPOINT_TYPE_CONTROL	0
#define USB_ENDPOINT_TYPE_ISOCHRONOUS	1
#define USB_ENDPOINT_TYPE_BULK		2
#define USB_ENDPOINT_TYPE_INTERRUPT	3

/* Interface descriptor */
#define USB_MAXINTERFACES	32
struct usb_interface_descriptor {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
  u_int8_t  bInterfaceNumber;
  u_int8_t  bAlternateSetting;
  u_int8_t  bNumEndpoints;
  u_int8_t  bInterfaceClass;
  u_int8_t  bInterfaceSubClass;
  u_int8_t  bInterfaceProtocol;
  u_int8_t  iInterface;

  struct usb_endpoint_descriptor *endpoint;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

#define USB_MAXALTSETTING	128	/* Hard limit */
struct usb_interface {
  struct usb_interface_descriptor *altsetting;

  int num_altsetting;
};

/* Configuration descriptor information.. */
#define USB_MAXCONFIG		8
struct usb_config_descriptor {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
  u_int16_t wTotalLength;
  u_int8_t  bNumInterfaces;
  u_int8_t  bConfigurationValue;
  u_int8_t  iConfiguration;
  u_int8_t  bmAttributes;
  u_int8_t  MaxPower;

  struct usb_interface *interface;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

/* Device descriptor */
struct usb_device_descriptor {
  u_int8_t  bLength;
  u_int8_t  bDescriptorType;
  u_int16_t bcdUSB;
  u_int8_t  bDeviceClass;
  u_int8_t  bDeviceSubClass;
  u_int8_t  bDeviceProtocol;
  u_int8_t  bMaxPacketSize0;
  u_int16_t idVendor;
  u_int16_t idProduct;
  u_int16_t bcdDevice;
  u_int8_t  iManufacturer;
  u_int8_t  iProduct;
  u_int8_t  iSerialNumber;
  u_int8_t  bNumConfigurations;
};

/*
 * Various libusb API related stuff
 */

#define USB_ENDPOINT_IN			0x80
#define USB_ENDPOINT_OUT		0x00

/* Error codes */
#define USB_ERROR_BEGIN			500000

/* Data types */
struct usb_device;
struct usb_bus;

/*
 * To maintain compatibility with applications already built with libusb,
 * we must only add entries to the end of this structure. NEVER delete or
 * move members and only change types if you really know what you're doing.
 */
struct usb_device {
  struct usb_device *next, *prev;

  char filename[PATH_MAX + 1];

  struct usb_bus *bus;

  struct usb_device_descriptor descriptor;
  struct usb_config_descriptor *config;

  void *dev;		/* Darwin support */

  u_int8_t devnum;

  unsigned char num_children;
  struct usb_device **children;
};

struct usb_bus {
  struct usb_bus *next, *prev;

  char dirname[PATH_MAX + 1];

  struct usb_device *devices;
  u_int32_t location;

  struct usb_device *root_dev;
};

/* Variables */
extern struct usb_bus *usb_busses;

#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */

char *usb_strerror(void);

void usb_init(void);
int usb_find_busses(void);
int usb_find_devices(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_H__ */

