/*
 * Core functions for libusb-compat-0.1
 * Copyright (C) 2008 Daniel Drake <dsd@gentoo.org>
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
 */

#include <config.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libusb.h>

#include "libusb-compat-usb.h"

static libusb_context *ctx = NULL;
static int usb_debug = 0;

struct usb_bus *usb_busses = NULL;

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

static void _usb_finalize(void)
{
  if (ctx) {
    libusb_exit(ctx);
    ctx = NULL;
  }
}

void usb_init(void)
{
  if (!ctx) {
    int r;
    r = libusb_init(&ctx);
    if (r < 0) {
      return;
    }

    /* usb_set_debug can be called before usb_init */
    if (usb_debug)
      libusb_set_debug(ctx, 3);

    atexit(_usb_finalize);
  }
}

char *usb_strerror(void)
{
  return strerror(errno);
}

static int find_busses(struct usb_bus **ret)
{
  libusb_device **dev_list = NULL;
  struct usb_bus *busses = NULL;
  struct usb_bus *bus;
  int dev_list_len = 0;
  int i;
  int r;

  r = libusb_get_device_list(ctx, &dev_list);
  if (r < 0) {
    return compat_err(r);
  }

  if (r == 0) {
    libusb_free_device_list(dev_list, 1);
    /* no buses */
    return 0;
  }

  /* iterate over the device list, identifying the individual busses.
   * we use the location field of the usb_bus structure to store the
   * bus number. */

  dev_list_len = r;
  for (i = 0; i < dev_list_len; i++) {
    libusb_device *dev = dev_list[i];
    uint8_t bus_num = libusb_get_bus_number(dev);

    /* if we already know about it, continue */
    if (busses) {
      bus = busses;
      int found = 0;
      do {
        if (bus_num == bus->location) {
          found = 1;
          break;
        }
      } while ((bus = bus->next) != NULL);
      if (found)
        continue;
    }

    /* add it to the list of busses */
    bus = malloc(sizeof(*bus));
    if (!bus)
      goto err;

    memset(bus, 0, sizeof(*bus));
    bus->location = bus_num;
    sprintf(bus->dirname, "%03d", bus_num);
    LIST_ADD(busses, bus);
  }

  libusb_free_device_list(dev_list, 1);
  *ret = busses;
  return 0;

err:
  bus = busses;
  while (bus) {
    struct usb_bus *tbus = bus->next;
    free(bus);
    bus = tbus;
  }
  return -ENOMEM;
}

int usb_find_busses(void)
{
  struct usb_bus *new_busses = NULL;
  struct usb_bus *bus;
  int changes = 0;
  int r;

  /* libusb-1.0 initialization might have failed, but we can't indicate
   * this with libusb-0.1, so trap that situation here */
  if (!ctx)
    return 0;

  r = find_busses(&new_busses);
  if (r < 0) {
    return r;
  }

  /* walk through all busses we already know about, removing duplicates
   * from the new list. if we do not find it in the new list, the bus
   * has been removed. */

  bus = usb_busses;
  while (bus) {
    struct usb_bus *tbus = bus->next;
    struct usb_bus *nbus = new_busses;
    int found = 0;

    while (nbus) {
      struct usb_bus *tnbus = nbus->next;

      if (bus->location == nbus->location) {
        LIST_DEL(new_busses, nbus);
        free(nbus);
        found = 1;
        break;
      }
      nbus = tnbus;
    }

    if (!found) {
      /* bus removed */
      changes++;
      LIST_DEL(usb_busses, bus);
      free(bus);
    }

    bus = tbus;
  }

  /* anything remaining in new_busses is a new bus */
  bus = new_busses;
  while (bus) {
    struct usb_bus *tbus = bus->next;
    LIST_DEL(new_busses, bus);
    LIST_ADD(usb_busses, bus);
    changes++;
    bus = tbus;
  }

  return changes;
}

static int find_devices(libusb_device **dev_list, int dev_list_len,
                        struct usb_bus *bus, struct usb_device **ret)
{
  struct usb_device *devices = NULL;
  struct usb_device *dev;
  int i;

  for (i = 0; i < dev_list_len; i++) {
    libusb_device *newlib_dev = dev_list[i];
    uint8_t bus_num = libusb_get_bus_number(newlib_dev);

    if (bus_num != bus->location)
      continue;

    dev = malloc(sizeof(*dev));
    if (!dev)
      goto err;

    /* No need to reference the device now, just take the pointer. We
     * increase the reference count later if we keep the device. */
    dev->dev = newlib_dev;

    dev->bus = bus;
    dev->devnum = libusb_get_device_address(newlib_dev);
    sprintf(dev->filename, "%03d", dev->devnum);
    LIST_ADD(devices, dev);
  }

  *ret = devices;
  return 0;

err:
  dev = devices;
  while (dev) {
    struct usb_device *tdev = dev->next;
    free(dev);
    dev = tdev;
  }
  return -ENOMEM;
}

static void clear_endpoint_descriptor(struct usb_endpoint_descriptor *ep)
{
  if (ep->extra)
    free(ep->extra);
}

static void clear_interface_descriptor(struct usb_interface_descriptor *iface)
{
  if (iface->extra)
    free(iface->extra);
  if (iface->endpoint) {
    int i;
    for (i = 0; i < iface->bNumEndpoints; i++)
      clear_endpoint_descriptor(iface->endpoint + i);
    free(iface->endpoint);
  }
}

static void clear_interface(struct usb_interface *iface)
{
  if (iface->altsetting) {
    int i;
    for (i = 0; i < iface->num_altsetting; i++)
      clear_interface_descriptor(iface->altsetting + i);
    free(iface->altsetting);
  }
}

static void clear_config_descriptor(struct usb_config_descriptor *config)
{
  if (config->extra)
    free(config->extra);
  if (config->interface) {
    int i;
    for (i = 0; i < config->bNumInterfaces; i++)
      clear_interface(config->interface + i);
    free(config->interface);
  }
}

static void clear_device(struct usb_device *dev)
{
  int i;
  for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
    clear_config_descriptor(dev->config + i);
}

static int copy_endpoint_descriptor(struct usb_endpoint_descriptor *dest,
                                    const struct libusb_endpoint_descriptor *src)
{
  memcpy(dest, src, USB_DT_ENDPOINT_AUDIO_SIZE);

  dest->extralen = src->extra_length;
  if (src->extra_length) {
    dest->extra = malloc(src->extra_length);
    if (!dest->extra)
      return -ENOMEM;
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int copy_interface_descriptor(struct usb_interface_descriptor *dest,
                                     const struct libusb_interface_descriptor *src)
{
  int i;
  int num_endpoints = src->bNumEndpoints;
  size_t alloc_size = sizeof(struct usb_endpoint_descriptor) * num_endpoints;

  memcpy(dest, src, USB_DT_INTERFACE_SIZE);
  dest->endpoint = malloc(alloc_size);
  if (!dest->endpoint)
    return -ENOMEM;
  memset(dest->endpoint, 0, alloc_size);

  for (i = 0; i < num_endpoints; i++) {
    int r = copy_endpoint_descriptor(dest->endpoint + i, &src->endpoint[i]);
    if (r < 0) {
      clear_interface_descriptor(dest);
      return r;
    }
  }

  dest->extralen = src->extra_length;
  if (src->extra_length) {
    dest->extra = malloc(src->extra_length);
    if (!dest->extra) {
      clear_interface_descriptor(dest);
      return -ENOMEM;
    }
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int copy_interface(struct usb_interface *dest,
                          const struct libusb_interface *src)
{
  int i;
  int num_altsetting = src->num_altsetting;
  size_t alloc_size = sizeof(struct usb_interface_descriptor)
                      * num_altsetting;

  dest->num_altsetting = num_altsetting;
  dest->altsetting = malloc(alloc_size);
  if (!dest->altsetting)
    return -ENOMEM;
  memset(dest->altsetting, 0, alloc_size);

  for (i = 0; i < num_altsetting; i++) {
    int r = copy_interface_descriptor(dest->altsetting + i,
                                      &src->altsetting[i]);
    if (r < 0) {
      clear_interface(dest);
      return r;
    }
  }

  return 0;
}

static int copy_config_descriptor(struct usb_config_descriptor *dest,
                                  const struct libusb_config_descriptor *src)
{
  int i;
  int num_interfaces = src->bNumInterfaces;
  size_t alloc_size = sizeof(struct usb_interface) * num_interfaces;

  memcpy(dest, src, USB_DT_CONFIG_SIZE);
  dest->interface = malloc(alloc_size);
  if (!dest->interface)
    return -ENOMEM;
  memset(dest->interface, 0, alloc_size);

  for (i = 0; i < num_interfaces; i++) {
    int r = copy_interface(dest->interface + i, &src->interface[i]);
    if (r < 0) {
      clear_config_descriptor(dest);
      return r;
    }
  }

  dest->extralen = src->extra_length;
  if (src->extra_length) {
    dest->extra = malloc(src->extra_length);
    if (!dest->extra) {
      clear_config_descriptor(dest);
      return -ENOMEM;
    }
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int initialize_device(struct usb_device *dev)
{
  libusb_device *newlib_dev = dev->dev;
  int num_configurations;
  size_t alloc_size;
  int r;
  int i;

  /* device descriptor is identical in both libs */
  r = libusb_get_device_descriptor(newlib_dev,
                                   (struct libusb_device_descriptor *) &dev->descriptor);
  if (r < 0) {
    return compat_err(r);
  }

  num_configurations = dev->descriptor.bNumConfigurations;
  alloc_size = sizeof(struct usb_config_descriptor) * num_configurations;
  dev->config = malloc(alloc_size);
  if (!dev->config)
    return -ENOMEM;
  memset(dev->config, 0, alloc_size);

  /* even though structures are identical, we can't just use libusb-1.0's
   * config descriptors because we have to store all configurations in
   * a single flat memory area (libusb-1.0 provides separate allocations).
   * we hand-copy libusb-1.0's descriptors into our own structures. */
  for (i = 0; i < num_configurations; i++) {
    struct libusb_config_descriptor *newlib_config;
    r = libusb_get_config_descriptor(newlib_dev, i, &newlib_config);
    if (r < 0) {
      clear_device(dev);
      free(dev->config);
      return compat_err(r);
    }
    r = copy_config_descriptor(dev->config + i, newlib_config);
    libusb_free_config_descriptor(newlib_config);
    if (r < 0) {
      clear_device(dev);
      free(dev->config);
      return r;
    }
  }

  /* libusb doesn't implement this and it doesn't seem that important. If
   * someone asks for it, we can implement it in v1.1 or later. */
  dev->num_children = 0;
  dev->children = NULL;

  libusb_ref_device(newlib_dev);
  return 0;
}

static void free_device(struct usb_device *dev)
{
  clear_device(dev);
  libusb_unref_device(dev->dev);
  free(dev);
}

int usb_find_devices(void)
{
  struct usb_bus *bus;
  libusb_device **dev_list;
  int dev_list_len;
  int changes = 0;

  /* libusb-1.0 initialization might have failed, but we can't indicate
   * this with libusb-0.1, so trap that situation here */
  if (!ctx)
    return 0;

  dev_list_len = libusb_get_device_list(ctx, &dev_list);
  if (dev_list_len < 0)
    return compat_err(dev_list_len);

  for (bus = usb_busses; bus; bus = bus->next) {
    int r;
    struct usb_device *new_devices = NULL;
    struct usb_device *dev;

    r = find_devices(dev_list, dev_list_len, bus, &new_devices);
    if (r < 0) {
      libusb_free_device_list(dev_list, 1);
      return r;
    }

    /* walk through the devices we already know about, removing duplicates
     * from the new list. if we do not find it in the new list, the device
     * has been removed. */
    dev = bus->devices;
    while (dev) {
      int found = 0;
      struct usb_device *tdev = dev->next;
      struct usb_device *ndev = new_devices;

      while (ndev) {
        if (ndev->devnum == dev->devnum) {
          LIST_DEL(new_devices, ndev);
          free(ndev);
          found = 1;
          break;
        }
        ndev = ndev->next;
      }

      if (!found) {
        LIST_DEL(bus->devices, dev);
        free_device(dev);
        changes++;
      }

      dev = tdev;
    }

    /* anything left in new_devices is a new device */
    dev = new_devices;
    while (dev) {
      struct usb_device *tdev = dev->next;
      r = initialize_device(dev);
      if (r < 0) {
        dev = tdev;
        continue;
      }
      LIST_DEL(new_devices, dev);
      LIST_ADD(bus->devices, dev);
      changes++;
      dev = tdev;
    }
  }

  libusb_free_device_list(dev_list, 1);
  return changes;
}

