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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "usbbus.h"
#include "log.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#define LOG_CATEGORY "libnfc.bus.usbbus"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

static libusb_context *ctx = NULL;

uint8_t get_usb_num_configs(struct libusb_device *dev);

int usbbus_prepare() {
  static bool usb_initialized = false;
  int res;
  if (!usb_initialized) {

#ifdef ENVVARS
    char *env_log_level = getenv("LIBNFC_LOG_LEVEL");
    // Set libusb debug only if asked explicitely:
    // LIBUSB_LOG_LEVEL=12288 (= NFC_LOG_PRIORITY_DEBUG * 2 ^ NFC_LOG_GROUP_LIBUSB)
    if (env_log_level
      && (((atoi(env_log_level) >> (NFC_LOG_GROUP_LIBUSB * 2)) & 0x00000003) >= NFC_LOG_PRIORITY_DEBUG)) {
      setenv("USB_DEBUG", "255", 1);
    }
#endif

    res = libusb_init(&ctx);
    if (res != 0) {
      log_put(LOG_GROUP,
              LOG_CATEGORY,
              NFC_LOG_PRIORITY_ERROR,
              "Unable to init libusb (%s)",
              libusb_strerror(res));
      return res;
    }
    usb_initialized = true;
  }

  // usb_find_devices will find all of the devices on each bus. This should be
  // called after usb_find_busses. Returns the number of changes since the
  // previous call to this function (total of new device and devices removed).
  libusb_device **tmp_devices;
  ssize_t num_devices = libusb_get_device_list(ctx, &tmp_devices);
  libusb_free_device_list(tmp_devices, (int) num_devices);
  if (num_devices <= 0) {
    log_put(LOG_GROUP,
            LOG_CATEGORY,
            NFC_LOG_PRIORITY_ERROR,
            "Unable to find USB devices (%s)",
            libusb_strerror((int) num_devices));
    return -1;
  }
  return 0;
}

size_t usbbus_usb_scan(nfc_connstring connstrings[],
                       const size_t connstrings_len,
                       struct usbbus_device *nfc_usb_devices,
                       const size_t num_nfc_usb_devices,
                       char *usb_driver_name) {
  usbbus_prepare();

  size_t device_found = 0;
  struct libusb_device **devices;
  ssize_t num_devices = libusb_get_device_list(ctx, &devices);
  for (size_t i = 0; i < num_devices; i++) {
    struct libusb_device *dev = devices[i];

    for (size_t nfc_dev_idx = 0; nfc_dev_idx < num_nfc_usb_devices; nfc_dev_idx++) {
      if (nfc_usb_devices[nfc_dev_idx].vendor_id == usbbus_get_vendor_id(dev)
        && nfc_usb_devices[nfc_dev_idx].product_id == usbbus_get_product_id(dev)) {

        size_t valid_config_idx = 1;

        // Make sure there are 2 endpoints available
        // with libusb-win32 we got some null pointers so be robust before looking at endpoints
        if (nfc_usb_devices[nfc_dev_idx].max_packet_size == 0) {

          bool found_valid_config = false;

          for (size_t config_idx = 0; config_idx < get_usb_num_configs(dev); i++) {
            struct libusb_config_descriptor *usb_config;
            int r = libusb_get_config_descriptor(dev, config_idx, &usb_config);

            if (r != 0
              || usb_config->interface == NULL
              || usb_config->interface->altsetting == NULL
              || usb_config->interface->altsetting->bNumEndpoints < 2) {
              // Nope, we maybe want the next one, let's try to find another
              libusb_free_config_descriptor(usb_config);
              continue;
            }

            libusb_free_config_descriptor(usb_config);

            found_valid_config = true;
            valid_config_idx = config_idx;
            break;
          }
          if (!found_valid_config) {
            libusb_unref_device(dev);
            continue;
          }
        }

        libusb_device_handle *udev;
        int res = libusb_open(dev, &udev);
        if (res < 0 && udev == NULL) {
          libusb_unref_device(dev);
          continue;
        }

        // Set configuration
        res = libusb_set_configuration(udev, (int) valid_config_idx);
        if (res < 0) {
          log_put(LOG_GROUP,
                  LOG_CATEGORY,
                  NFC_LOG_PRIORITY_ERROR,
                  "Unable to set USB configuration (%s)",
                  libusb_strerror(res));
          libusb_close(udev);
          libusb_unref_device(dev);
          // we failed to use the device
          continue;
        }

        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "device found: Vendor-Id: %d Product-Id %d",
                usbbus_get_vendor_id(dev), usbbus_get_product_id(dev));
        libusb_close(udev);

        uint8_t dev_address = libusb_get_device_address(dev);
        printf("%s:%03d:%03d",
               "Test",
               dev_address,
               (int) valid_config_idx);
        size_t size_new_str = snprintf(
          connstrings[device_found],
          sizeof(connstrings[device_found]),
          "%s:%03d:%03d",
          usb_driver_name,
          dev_address,
          (int) valid_config_idx);

        if (size_new_str >= (int) sizeof(nfc_connstring)) {
          // truncation occurred, skipping that one
          libusb_unref_device(dev);
          continue;
        }
        device_found++;
        // Test if we reach the maximum "wanted" devices
        if (device_found == connstrings_len) {
          libusb_free_device_list(devices, 0);
          return device_found;
        }
      }
    }
  }
  libusb_free_device_list(devices, 0);
  return device_found;
}

void usbbus_get_usb_endpoints(struct libusb_device *dev,
                              uint8_t *endpoint_in,
                              uint8_t *endpoint_out,
                              uint16_t *max_packet_size) {

  bool endpoint_in_set = false;
  bool endpoint_out_set = false;
  size_t num_configs = get_usb_num_configs(dev);
  for (size_t config_idx = 0; config_idx < num_configs; config_idx++) {
    struct libusb_config_descriptor *usb_config;

    int r = libusb_get_config_descriptor(dev, config_idx, &usb_config);
    if (r != 0) {
      continue;
    }

    if (!usb_config->interface) {
      continue;
    }
    for (size_t interface_idx = 0; interface_idx < usb_config->bNumInterfaces; interface_idx++) {
      struct libusb_interface interface = usb_config->interface[interface_idx];
      if (!interface.altsetting) {
        continue;
      }
      for (size_t settings_idx = 0; settings_idx < interface.num_altsetting; settings_idx++) {
        struct libusb_interface_descriptor settings = interface.altsetting[settings_idx];
        if (!settings.endpoint) {
          continue;
        }

        // 3 Endpoints maximum: Interrupt In, Bulk In, Bulk Out
        for (size_t endpoint_idx = 0; endpoint_idx < settings.bNumEndpoints; endpoint_idx++) {
          struct libusb_endpoint_descriptor endpoint = settings.endpoint[endpoint_idx];

          // Only accept bulk transfer endpoints (ignore interrupt endpoints)
          if (endpoint.bmAttributes != LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK) {
            continue;
          }

          // Copy the endpoint to a local var, makes it more readable code
          uint8_t endpoint_address = endpoint.bEndpointAddress;

          // Test if we dealing with a bulk IN endpoint
          if ((endpoint_address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN && !endpoint_in_set) {
            *endpoint_in = endpoint_address;
            *max_packet_size = endpoint.wMaxPacketSize;
            endpoint_in_set = true;
          }
          // Test if we dealing with a bulk OUT endpoint
          if ((endpoint_address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT && !endpoint_out_set) {
            *endpoint_out = endpoint_address;
            *max_packet_size = endpoint.wMaxPacketSize;
            endpoint_out_set = true;
          }

          if (endpoint_in_set && endpoint_out_set) {
            libusb_free_config_descriptor(usb_config);
            return;
          }
        }
      }
    }

    libusb_free_config_descriptor(usb_config);
  }
}

uint8_t get_usb_num_configs(struct libusb_device *dev) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  return descriptor.bNumConfigurations;
}

void usbbus_get_usb_device_name(struct libusb_device *dev, libusb_device_handle *udev, char *buffer, size_t len) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  if (descriptor.iManufacturer || descriptor.iProduct) {
    if (udev) {
      libusb_get_string_descriptor_ascii(udev, descriptor.iManufacturer & 0xff, (unsigned char *) buffer, len);
      if (strlen(buffer) > 0) {
        strncpy(buffer + strlen(buffer), " / ", 4);
      }
      libusb_get_string_descriptor_ascii(udev,
                                         descriptor.iProduct & 0xff,
                                         (unsigned char *) buffer + strlen(buffer),
                                         len - strlen(buffer));
    }
  }
}


void usbbus_get_device(uint8_t dev_address, struct libusb_device ** dev, struct libusb_device_handle ** dev_handle) {
  struct libusb_device ** device_list;
  ssize_t num_devices = libusb_get_device_list(ctx, &device_list);
  for (size_t i = 0; i < num_devices; i++) {
    if (libusb_get_device_address(device_list[i]) != dev_address) {
      continue;
    } else {
      *dev = device_list[i];
      int res = libusb_open(*dev, dev_handle);
      if (res != 0 || dev_handle == NULL) {
        log_put(LOG_GROUP,LOG_CATEGORY,NFC_LOG_PRIORITY_ERROR,
               "Unable to open libusb device (%s)",libusb_strerror(res));
        continue;
      }
    }
  }

  // libusb works with a reference counter which is set to 1 for each device when calling libusb_get_device_list and increased
  // by libusb_open. Thus we decrease the counter by 1 for all devices and only the "real" device will survive
  libusb_free_device_list(device_list, num_devices);
}


void usbbus_close(struct libusb_device * dev, struct libusb_device_handle * dev_handle) {
  libusb_close(dev_handle);
  libusb_unref_device(dev);
  libusb_exit(ctx);
}

uint16_t usbbus_get_vendor_id(struct libusb_device *dev) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  return descriptor.idVendor;
}

uint16_t usbbus_get_product_id(struct libusb_device *dev) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  return descriptor.idProduct;
}


int usbbus_get_num_alternate_settings(struct libusb_device *dev, uint8_t config_idx) {
  struct libusb_config_descriptor *usb_config;
  int r = libusb_get_config_descriptor(dev, config_idx, &usb_config);
  if (r != 0 || usb_config == NULL) {
    return -1;
  }
  libusb_free_config_descriptor(usb_config);
  return usb_config->interface->num_altsetting;
}



