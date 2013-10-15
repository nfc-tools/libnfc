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
#include <libusb.h>
#include <stdint.h>
#include "usbbus.h"
#include "log.h"
#define LOG_CATEGORY "libnfc.buses.usbbus"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

/*
 * This file embeds partially libusb-compat-0.1 by:
 * Copyright (C) 2008 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2000-2003 Johannes Erdfelt <johannes@erdfelt.com>
 * This layer will be removed ASAP before integration in the main trunk
 */


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

#define USBBUS_DT_CONFIG_SIZE 9
#define USBBUS_DT_INTERFACE_SIZE 9
#define USBBUS_DT_ENDPOINT_AUDIO_SIZE 9

static libusb_context *ctx = NULL;

struct usbbus_bus *usb_busses = NULL;

static void _usb_finalize(void)
{
  if (ctx) {
    libusb_exit(ctx);
    ctx = NULL;
  }
}

static void usb_init(void)
{
  if (!ctx) {
    int r;
    r = libusb_init(&ctx);
    if (r < 0) {
      return;
    }

    atexit(_usb_finalize);
  }
}

static int find_busses(struct usbbus_bus **ret)
{
  libusb_device **dev_list = NULL;
  struct usbbus_bus *busses = NULL;
  struct usbbus_bus *bus;
  int dev_list_len = 0;
  int i;
  int r;

  r = libusb_get_device_list(ctx, &dev_list);
  if (r < 0) {
    return r;
  }

  if (r == 0) {
    libusb_free_device_list(dev_list, 1);
    /* no buses */
    return 0;
  }

  /* iterate over the device list, identifying the individual busses.
   * we use the location field of the usbbus_bus structure to store the
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
    snprintf(bus->dirname, USBBUS_PATH_MAX, "%03d", bus_num);
    LIST_ADD(busses, bus);
  }

  libusb_free_device_list(dev_list, 1);
  *ret = busses;
  return 0;

err:
  bus = busses;
  while (bus) {
    struct usbbus_bus *tbus = bus->next;
    free(bus);
    bus = tbus;
  }
  return LIBUSB_ERROR_NO_MEM;
}

static int usb_find_busses(void)
{
  struct usbbus_bus *new_busses = NULL;
  struct usbbus_bus *bus;
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
    struct usbbus_bus *tbus = bus->next;
    struct usbbus_bus *nbus = new_busses;
    int found = 0;

    while (nbus) {
      struct usbbus_bus *tnbus = nbus->next;

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
    struct usbbus_bus *tbus = bus->next;
    LIST_DEL(new_busses, bus);
    LIST_ADD(usb_busses, bus);
    changes++;
    bus = tbus;
  }

  return changes;
}

static int find_devices(libusb_device **dev_list, int dev_list_len,
                        struct usbbus_bus *bus, struct usbbus_device **ret)
{
  struct usbbus_device *devices = NULL;
  struct usbbus_device *dev;
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
    snprintf(dev->filename, USBBUS_PATH_MAX, "%03d", dev->devnum);
    LIST_ADD(devices, dev);
  }

  *ret = devices;
  return 0;

err:
  dev = devices;
  while (dev) {
    struct usbbus_device *tdev = dev->next;
    free(dev);
    dev = tdev;
  }
  return LIBUSB_ERROR_NO_MEM;
}

static void clear_endpoint_descriptor(struct usbbus_endpoint_descriptor *ep)
{
  if (ep->extra)
    free(ep->extra);
}

static void clear_interface_descriptor(struct usbbus_interface_descriptor *iface)
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

static void clear_interface(struct usbbus_interface *iface)
{
  if (iface->altsetting) {
    int i;
    for (i = 0; i < iface->num_altsetting; i++)
      clear_interface_descriptor(iface->altsetting + i);
    free(iface->altsetting);
  }
}

static void clear_config_descriptor(struct usbbus_config_descriptor *config)
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

static void clear_device(struct usbbus_device *dev)
{
  int i;
  for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
    clear_config_descriptor(dev->config + i);
}

static int copy_endpoint_descriptor(struct usbbus_endpoint_descriptor *dest,
                                    const struct libusb_endpoint_descriptor *src)
{
  memcpy(dest, src, USBBUS_DT_ENDPOINT_AUDIO_SIZE);

  dest->extralen = src->extra_length;
  if (src->extra_length) {
    dest->extra = malloc(src->extra_length);
    if (!dest->extra)
      return LIBUSB_ERROR_NO_MEM;
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int copy_interface_descriptor(struct usbbus_interface_descriptor *dest,
                                     const struct libusb_interface_descriptor *src)
{
  int i;
  int num_endpoints = src->bNumEndpoints;
  size_t alloc_size = sizeof(struct usbbus_endpoint_descriptor) * num_endpoints;

  memcpy(dest, src, USBBUS_DT_INTERFACE_SIZE);
  dest->endpoint = malloc(alloc_size);
  if (!dest->endpoint)
    return LIBUSB_ERROR_NO_MEM;
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
      return LIBUSB_ERROR_NO_MEM;
    }
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int copy_interface(struct usbbus_interface *dest,
                          const struct libusb_interface *src)
{
  int i;
  int num_altsetting = src->num_altsetting;
  size_t alloc_size = sizeof(struct usbbus_interface_descriptor)
                      * num_altsetting;

  dest->num_altsetting = num_altsetting;
  dest->altsetting = malloc(alloc_size);
  if (!dest->altsetting)
    return LIBUSB_ERROR_NO_MEM;
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

static int copy_config_descriptor(struct usbbus_config_descriptor *dest,
                                  const struct libusb_config_descriptor *src)
{
  int i;
  int num_interfaces = src->bNumInterfaces;
  size_t alloc_size = sizeof(struct usbbus_interface) * num_interfaces;

  memcpy(dest, src, USBBUS_DT_CONFIG_SIZE);
  dest->interface = malloc(alloc_size);
  if (!dest->interface)
    return LIBUSB_ERROR_NO_MEM;
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
      return LIBUSB_ERROR_NO_MEM;
    }
    memcpy(dest->extra, src->extra, src->extra_length);
  }

  return 0;
}

static int initialize_device(struct usbbus_device *dev)
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
    return r;
  }

  num_configurations = dev->descriptor.bNumConfigurations;
  alloc_size = sizeof(struct usbbus_config_descriptor) * num_configurations;
  dev->config = malloc(alloc_size);
  if (!dev->config)
    return LIBUSB_ERROR_NO_MEM;
  memset(dev->config, 0, alloc_size);

  for (i = 0; i < num_configurations; i++) {
    struct libusb_config_descriptor *newlib_config;
    r = libusb_get_config_descriptor(newlib_dev, i, &newlib_config);
    if (r < 0) {
      clear_device(dev);
      free(dev->config);
      return r;
    }
    r = copy_config_descriptor(dev->config + i, newlib_config);
    libusb_free_config_descriptor(newlib_config);
    if (r < 0) {
      clear_device(dev);
      free(dev->config);
      return r;
    }
  }
  dev->num_children = 0;
  dev->children = NULL;

  libusb_ref_device(newlib_dev);
  return 0;
}

static void free_device(struct usbbus_device *dev)
{
  clear_device(dev);
  libusb_unref_device(dev->dev);
  free(dev);
}

static int usb_find_devices(void)
{
  struct usbbus_bus *bus;
  libusb_device **dev_list;
  int dev_list_len;
  int changes = 0;

  /* libusb-1.0 initialization might have failed, but we can't indicate
   * this with libusb-0.1, so trap that situation here */
  if (!ctx)
    return 0;

  dev_list_len = libusb_get_device_list(ctx, &dev_list);
  if (dev_list_len < 0)
    return dev_list_len;

  for (bus = usb_busses; bus; bus = bus->next) {
    int r;
    struct usbbus_device *new_devices = NULL;
    struct usbbus_device *dev;

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
      struct usbbus_device *tdev = dev->next;
      struct usbbus_device *ndev = new_devices;

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
      struct usbbus_device *tdev = dev->next;
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

usbbus_device_handle *usbbus_open(struct usbbus_device *dev)
{
  int r;
  usbbus_device_handle *udev;
  r = libusb_open((libusb_device *) dev->dev, (libusb_device_handle **)&udev);
  if (r < 0) {
    return NULL;
  }
  return (usbbus_device_handle *)udev;
}

void usbbus_close(usbbus_device_handle *dev)
{
  libusb_close((libusb_device_handle *)dev);
}

int usbbus_set_configuration(usbbus_device_handle *dev, int configuration)
{
  return libusb_set_configuration((libusb_device_handle *)dev, configuration);
}

int usbbus_get_string_simple(usbbus_device_handle *dev, int index, char *buf, size_t buflen)
{
  return libusb_get_string_descriptor_ascii((libusb_device_handle *)dev, index & 0xff,
                                            (unsigned char *) buf, (int) buflen);
}

int usbbus_bulk_transfer(usbbus_device_handle *dev, int ep, char *bytes, int size, int *actual_length, int timeout)
{
  return libusb_bulk_transfer((libusb_device_handle *)dev, ep & 0xff, (unsigned char *)bytes, size, actual_length, timeout);
}

int usbbus_claim_interface(usbbus_device_handle *dev, int interface)
{
  return libusb_claim_interface((libusb_device_handle *)dev, interface);
}

int usbbus_release_interface(usbbus_device_handle *dev, int interface)
{
  return libusb_release_interface((libusb_device_handle *)dev, interface);
}

int usbbus_set_interface_alt_setting(usbbus_device_handle *dev, int interface, int alternate)
{
  return libusb_set_interface_alt_setting((libusb_device_handle *)dev, interface, alternate);
}

int usbbus_reset(usbbus_device_handle *dev)
{
  return libusb_reset_device((libusb_device_handle *)dev);
}

const char *usbbus_strerror(int errcode)
{
  return libusb_strerror((enum libusb_error)errcode);
}

struct usbbus_bus *usbbus_get_busses(void)
{
  return usb_busses;
}

