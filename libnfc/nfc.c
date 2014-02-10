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
 */

/**
 * @file nfc.c
 * @brief NFC library implementation
 */
/**
 * @defgroup lib Library initialization/deinitialization
 * This page details how to initialize and deinitialize libnfc. Initialization
 * must be performed before using any libnfc functionality, and similarly you
 * must not call any libnfc functions after deinitialization.
 */
/**
 * @defgroup dev NFC Device/Hardware manipulation
 * The functionality documented below is designed to help with the following
 * operations:
 * - Enumerating the NFC devices currently attached to the system
 * - Opening and closing the chosen device
 */
/**
 * @defgroup initiator  NFC initiator
 * This page details how to act as "reader".
 */
/**
 * @defgroup target  NFC target
 * This page details how to act as tag (i.e. MIFARE Classic) or NFC target device.
 */
/**
 * @defgroup error  Error reporting
 * Most libnfc functions return 0 on success or one of error codes defined on failure.
 */
/**
 * @defgroup data  Special data accessors
 * The functionnality documented below allow to access to special data as device name or device connstring.
 */
/**
 * @defgroup properties  Properties accessors
 * The functionnality documented below allow to configure parameters and registers.
 */
/**
 * @defgroup misc Miscellaneous
 *
 */
/**
 * @defgroup string-converter  To-string converters
 * The functionnality documented below allow to retreive some information in text format.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-internal.h"
#include "target-subr.h"
#include "drivers.h"

#if defined (DRIVER_ACR122_PCSC_ENABLED)
#  include "drivers/acr122_pcsc.h"
#endif /* DRIVER_ACR122_PCSC_ENABLED */

#if defined (DRIVER_ACR122_USB_ENABLED)
#  include "drivers/acr122_usb.h"
#endif /* DRIVER_ACR122_USB_ENABLED */

#if defined (DRIVER_ACR122S_ENABLED)
#  include "drivers/acr122s.h"
#endif /* DRIVER_ACR122S_ENABLED */

#if defined (DRIVER_PN53X_USB_ENABLED)
#  include "drivers/pn53x_usb.h"
#endif /* DRIVER_PN53X_USB_ENABLED */

#if defined (DRIVER_ARYGON_ENABLED)
#  include "drivers/arygon.h"
#endif /* DRIVER_ARYGON_ENABLED */

#if defined (DRIVER_PN532_UART_ENABLED)
#  include "drivers/pn532_uart.h"
#endif /* DRIVER_PN532_UART_ENABLED */

#if defined (DRIVER_PN532_SPI_ENABLED)
#  include "drivers/pn532_spi.h"
#endif /* DRIVER_PN532_SPI_ENABLED */

#if defined (DRIVER_PN532_I2C_ENABLED)
#  include "drivers/pn532_i2c.h"
#endif /* DRIVER_PN532_I2C_ENABLED */


#define LOG_CATEGORY "libnfc.general"
#define LOG_GROUP    NFC_LOG_GROUP_GENERAL

struct nfc_driver_list {
  const struct nfc_driver_list *next;
  const struct nfc_driver *driver;
};

const struct nfc_driver_list *nfc_drivers = NULL;

static void
nfc_drivers_init(void)
{
#if defined (DRIVER_PN53X_USB_ENABLED)
  nfc_register_driver(&pn53x_usb_driver);
#endif /* DRIVER_PN53X_USB_ENABLED */
#if defined (DRIVER_ACR122_PCSC_ENABLED)
  nfc_register_driver(&acr122_pcsc_driver);
#endif /* DRIVER_ACR122_PCSC_ENABLED */
#if defined (DRIVER_ACR122_USB_ENABLED)
  nfc_register_driver(&acr122_usb_driver);
#endif /* DRIVER_ACR122_USB_ENABLED */
#if defined (DRIVER_ACR122S_ENABLED)
  nfc_register_driver(&acr122s_driver);
#endif /* DRIVER_ACR122S_ENABLED */
#if defined (DRIVER_PN532_UART_ENABLED)
  nfc_register_driver(&pn532_uart_driver);
#endif /* DRIVER_PN532_UART_ENABLED */
#if defined (DRIVER_PN532_SPI_ENABLED)
  nfc_register_driver(&pn532_spi_driver);
#endif /* DRIVER_PN532_SPI_ENABLED */
#if defined (DRIVER_PN532_I2C_ENABLED)
  nfc_register_driver(&pn532_i2c_driver);
#endif /* DRIVER_PN532_I2C_ENABLED */
#if defined (DRIVER_ARYGON_ENABLED)
  nfc_register_driver(&arygon_driver);
#endif /* DRIVER_ARYGON_ENABLED */
}


/** @ingroup lib
 * @brief Register an NFC device driver with libnfc.
 * This function registers a driver with libnfc, the caller is responsible of managing the lifetime of the
 * driver and make sure that any resources associated with the driver are available after registration.
 * @param pnd Pointer to an NFC device driver to be registered.
 * @retval NFC_SUCCESS If the driver registration succeeds.
 */
int
nfc_register_driver(const struct nfc_driver *ndr)
{
  if (!ndr)
    return NFC_EINVARG;

  struct nfc_driver_list *pndl = (struct nfc_driver_list *)malloc(sizeof(struct nfc_driver_list));
  if (!pndl)
    return NFC_ESOFT;

  pndl->driver = ndr;
  pndl->next = nfc_drivers;
  nfc_drivers = pndl;

  return NFC_SUCCESS;
}

/** @ingroup lib
 * @brief Initialize libnfc.
 * This function must be called before calling any other libnfc function
 * @param context Output location for nfc_context
 */
void
nfc_init(nfc_context **context)
{
  *context = nfc_context_new();
  if (!*context) {
    perror("malloc");
    return;
  }
  if (!nfc_drivers)
    nfc_drivers_init();
}

/** @ingroup lib
 * @brief Deinitialize libnfc.
 * Should be called after closing all open devices and before your application terminates.
 * @param context The context to deinitialize
 */
void
nfc_exit(nfc_context *context)
{
  while (nfc_drivers) {
    struct nfc_driver_list *pndl = (struct nfc_driver_list *) nfc_drivers;
    nfc_drivers = pndl->next;
    free(pndl);
  }

  nfc_context_free(context);
}

/** @ingroup dev
 * @brief Open a NFC device
 * @param context The context to operate on.
 * @param connstring The device connection string if specific device is wanted, \c NULL otherwise
 * @return Returns pointer to a \a nfc_device struct if successfull; otherwise returns \c NULL value.
 *
 * If \e connstring is \c NULL, the first available device from \a nfc_list_devices function is used.
 *
 * If \e connstring is set, this function will try to claim the right device using information provided by \e connstring.
 *
 * When it has successfully claimed a NFC device, memory is allocated to save the device information.
 * It will return a pointer to a \a nfc_device struct.
 * This pointer should be supplied by every next functions of libnfc that should perform an action with this device.
 *
 * @note Depending on the desired operation mode, the device needs to be configured by using nfc_initiator_init() or nfc_target_init(),
 * optionally followed by manual tuning of the parameters if the default parameters are not suiting your goals.
 */
nfc_device *
nfc_open(nfc_context *context, const nfc_connstring connstring)
{
  nfc_device *pnd = NULL;

  nfc_connstring ncs;
  if (connstring == NULL) {
    if (!nfc_list_devices(context, &ncs, 1)) {
      return NULL;
    }
  } else {
    strncpy(ncs, connstring, sizeof(nfc_connstring));
    ncs[sizeof(nfc_connstring) - 1] = '\0';
  }

  // Search through the device list for an available device
  const struct nfc_driver_list *pndl = nfc_drivers;
  while (pndl) {
    const struct nfc_driver *ndr = pndl->driver;

    // Specific device is requested: using device description
    if (0 != strncmp(ndr->name, ncs, strlen(ndr->name))) {
      // Check if connstring driver is usb -> accept any driver *_usb
      if ((0 != strncmp("usb", ncs, strlen("usb"))) || 0 != strncmp("_usb", ndr->name + (strlen(ndr->name) - 4), 4)) {
        pndl = pndl->next;
        continue;
      }
    }

    pnd = ndr->open(context, ncs);
    // Test if the opening was successful
    if (pnd == NULL) {
      if (0 == strncmp("usb", ncs, strlen("usb"))) {
        // We've to test the other usb drivers before giving up
        pndl = pndl->next;
        continue;
      }
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Unable to open \"%s\".", ncs);
      return NULL;
    }
    for (uint32_t i = 0; i > context->user_defined_device_count; i++) {
      if (strcmp(ncs, context->user_defined_devices[i].connstring) == 0) {
        // This is a device sets by user, we use the device name given by user
        strcpy(pnd->name, context->user_defined_devices[i].name);
        break;
      }
    }
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "\"%s\" (%s) has been claimed.", pnd->name, pnd->connstring);
    return pnd;
  }

  // Too bad, no driver can decode connstring
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "No driver available to handle \"%s\".", ncs);
  return NULL;
}

/** @ingroup dev
 * @brief Close from a NFC device
 * @param pnd \a nfc_device struct pointer that represent currently used device
 *
 * Initiator's selected tag is closed and the device, including allocated \a nfc_device struct, is released.
 */
void
nfc_close(nfc_device *pnd)
{
  if (pnd) {
    // Close, clean up and release the device
    pnd->driver->close(pnd);
  }
}

/** @ingroup dev
 * @brief Scan for discoverable supported devices (ie. only available for some drivers)
 * @return Returns the number of devices found.
 * @param context The context to operate on, or NULL for the default context.
 * @param connstrings array of \a nfc_connstring.
 * @param connstrings_len size of the \a connstrings array.
 *
 */
size_t
nfc_list_devices(nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;

#ifdef CONFFILES
  // Load manually configured devices (from config file and env variables)
  // TODO From env var...
  for (uint32_t i = 0; i < context->user_defined_device_count; i++) {
    if (context->user_defined_devices[i].optional) {
      // let's make sure the device exists
      nfc_device *pnd = NULL;

#ifdef ENVVARS
      char *env_log_level = getenv("LIBNFC_LOG_LEVEL");
      char *old_env_log_level = NULL;
      // do it silently
      if (env_log_level) {
        if ((old_env_log_level = malloc(strlen(env_log_level) + 1)) == NULL) {
          log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to malloc()");
          return 0;
        }
        strcpy(old_env_log_level, env_log_level);
      }
      setenv("LIBNFC_LOG_LEVEL", "0", 1);
#endif // ENVVARS

      pnd = nfc_open(context, context->user_defined_devices[i].connstring);

#ifdef ENVVARS
      if (old_env_log_level) {
        setenv("LIBNFC_LOG_LEVEL", old_env_log_level, 1);
        free(old_env_log_level);
      } else {
        unsetenv("LIBNFC_LOG_LEVEL");
      }
#endif // ENVVARS

      if (pnd) {
        nfc_close(pnd);
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "User device %s found", context->user_defined_devices[i].name);
        strcpy((char *)(connstrings + device_found), context->user_defined_devices[i].connstring);
        device_found ++;
        if (device_found == connstrings_len)
          break;
      }
    } else {
      // manual choice is not marked as optional so let's take it blindly
      strcpy((char *)(connstrings + device_found), context->user_defined_devices[i].connstring);
      device_found++;
      if (device_found >= connstrings_len)
        return device_found;
    }
  }
#endif // CONFFILES

  // Device auto-detection
  if (context->allow_autoscan) {
    const struct nfc_driver_list *pndl = nfc_drivers;
    while (pndl) {
      const struct nfc_driver *ndr = pndl->driver;
      if ((ndr->scan_type == NOT_INTRUSIVE) || ((context->allow_intrusive_scan) && (ndr->scan_type == INTRUSIVE))) {
        size_t _device_found = ndr->scan(context, connstrings + (device_found), connstrings_len - (device_found));
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%ld device(s) found using %s driver", (unsigned long) _device_found, ndr->name);
        if (_device_found > 0) {
          device_found += _device_found;
          if (device_found == connstrings_len)
            break;
        }
      } // scan_type is INTRUSIVE but not allowed or NOT_AVAILABLE
      pndl = pndl->next;
    }
  } else if (context->user_defined_device_count == 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "Warning: %s" , "user must specify device(s) manually when autoscan is disabled");
  }

  return device_found;
}

/** @ingroup properties
 * @brief Set a device's integer-property value
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param property \a nfc_property which will be set
 * @param value integer value
 *
 * Sets integer property.
 *
 * @see nfc_property enum values
 */
int
nfc_device_set_property_int(nfc_device *pnd, const nfc_property property, const int value)
{
  HAL(device_set_property_int, pnd, property, value);
}


/** @ingroup properties
 * @brief Set a device's boolean-property value
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param property \a nfc_property which will be set
 * @param bEnable boolean to activate/disactivate the property
 *
 * Configures parameters and registers that control for example timing,
 * modulation, frame and error handling.  There are different categories for
 * configuring the \e PN53X chip features (handle, activate, infinite and
 * accept).
 */
int
nfc_device_set_property_bool(nfc_device *pnd, const nfc_property property, const bool bEnable)
{
  HAL(device_set_property_bool, pnd, property, bEnable);
}

/** @ingroup initiator
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 *
 * The NFC device is configured to function as RFID reader.
 * After initialization it can be used to communicate to passive RFID tags and active NFC devices.
 * The reader will act as initiator to communicate peer 2 peer (NFCIP) to other active NFC devices.
 * - Crc is handled by the device (NP_HANDLE_CRC = true)
 * - Parity is handled the device (NP_HANDLE_PARITY = true)
 * - Cryto1 cipher is disabled (NP_ACTIVATE_CRYPTO1 = false)
 * - Easy framing is enabled (NP_EASY_FRAMING = true)
 * - Auto-switching in ISO14443-4 mode is enabled (NP_AUTO_ISO14443_4 = true)
 * - Invalid frames are not accepted (NP_ACCEPT_INVALID_FRAMES = false)
 * - Multiple frames are not accepted (NP_ACCEPT_MULTIPLE_FRAMES = false)
 * - 14443-A mode is activated (NP_FORCE_ISO14443_A = true)
 * - speed is set to 106 kbps (NP_FORCE_SPEED_106 = true)
 * - Let the device try forever to find a target (NP_INFINITE_SELECT = true)
 * - RF field is shortly dropped (if it was enabled) then activated again
 */
int
nfc_initiator_init(nfc_device *pnd)
{
  int res = 0;
  // Drop the field for a while
  if ((res = nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false)) < 0)
    return res;
  // Enable field so more power consuming cards can power themselves up
  if ((res = nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true)) < 0)
    return res;
  // Let the device try forever to find a target/tag
  if ((res = nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, true)) < 0)
    return res;
  // Activate auto ISO14443-4 switching by default
  if ((res = nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, true)) < 0)
    return res;
  // Force 14443-A mode
  if ((res = nfc_device_set_property_bool(pnd, NP_FORCE_ISO14443_A, true)) < 0)
    return res;
  // Force speed at 106kbps
  if ((res = nfc_device_set_property_bool(pnd, NP_FORCE_SPEED_106, true)) < 0)
    return res;
  // Disallow invalid frame
  if ((res = nfc_device_set_property_bool(pnd, NP_ACCEPT_INVALID_FRAMES, false)) < 0)
    return res;
  // Disallow multiple frames
  if ((res = nfc_device_set_property_bool(pnd, NP_ACCEPT_MULTIPLE_FRAMES, false)) < 0)
    return res;
  HAL(initiator_init, pnd);
}

/** @ingroup initiator
 * @brief Initialize NFC device as initiator with its secure element initiator (reader)
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 *
 * The NFC device is configured to function as secure element reader.
 * After initialization it can be used to communicate with the secure element.
 * @note RF field is desactvated in order to some power
 */
int
nfc_initiator_init_secure_element(nfc_device *pnd)
{
  HAL(initiator_init_secure_element, pnd);
}

/** @ingroup initiator
 * @brief Select a passive or emulated tag
 * @return Returns selected passive target count on success, otherwise returns libnfc's error code (negative value)
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param nm desired modulation
 * @param pbtInitData optional initiator data, NULL for using the default values.
 * @param szInitData length of initiator data \a pbtInitData.
 * @note pbtInitData is used with different kind of data depending on modulation type:
 * - for an ISO/IEC 14443 type A modulation, pbbInitData contains the UID you want to select;
 * - for an ISO/IEC 14443 type B modulation, pbbInitData contains Application Family Identifier (AFI) (see ISO/IEC 14443-3)
        and optionally a second byte = 0x01 if you want to use probabilistic approach instead of timeslot approach;
 * - for a FeliCa modulation, pbbInitData contains a 5-byte polling payload (see ISO/IEC 18092 11.2.2.5).
 * - for ISO14443B', ASK CTx and ST SRx, see corresponding standards
 * - if NULL, default values adequate for the chosen modulation will be used.
 *
 * @param[out] pnt \a nfc_target struct pointer which will filled if available
 *
 * The NFC device will try to find one available passive tag or emulated tag.
 *
 * The chip needs to know with what kind of tag it is dealing with, therefore
 * the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 */
int
nfc_initiator_select_passive_target(nfc_device *pnd,
                                    const nfc_modulation nm,
                                    const uint8_t *pbtInitData, const size_t szInitData,
                                    nfc_target *pnt)
{
  uint8_t *abtInit = NULL;
  uint8_t abtTmpInit[MAX(12, szInitData)];
  size_t  szInit = 0;
  if (szInitData == 0) {
    // Provide default values, if any
    prepare_initiator_data(nm, &abtInit, &szInit);
  } else if (nm.nmt == NMT_ISO14443A) {
    abtInit = abtTmpInit;
    iso14443_cascade_uid(pbtInitData, szInitData, abtInit, &szInit);
  } else {
    abtInit = abtTmpInit;
    memcpy(abtInit, pbtInitData, szInitData);
    szInit = szInitData;
  }

  HAL(initiator_select_passive_target, pnd, nm, abtInit, szInit, pnt);
}

/** @ingroup initiator
 * @brief List passive or emulated tags
 * @return Returns the number of targets found on success, otherwise returns libnfc's error code (negative value)
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param nm desired modulation
 * @param[out] ant array of \a nfc_target that will be filled with targets info
 * @param szTargets size of \a ant (will be the max targets listed)
 *
 * The NFC device will try to find the available passive tags. Some NFC devices
 * are capable to emulate passive tags. The standards (ISO18092 and ECMA-340)
 * describe the modulation that can be used for reader to passive
 * communications. The chip needs to know with what kind of tag it is dealing
 * with, therefore the initial modulation and speed (106, 212 or 424 kbps)
 * should be supplied.
 */
int
nfc_initiator_list_passive_targets(nfc_device *pnd,
                                   const nfc_modulation nm,
                                   nfc_target ant[], const size_t szTargets)
{
  nfc_target nt;
  size_t  szTargetFound = 0;
  uint8_t *pbtInitData = NULL;
  size_t  szInitDataLen = 0;
  int res = 0;

  pnd->last_error = 0;

  // Let the reader only try once to find a tag
  bool bInfiniteSelect = pnd->bInfiniteSelect;
  if ((res = nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false)) < 0) {
    return res;
  }

  prepare_initiator_data(nm, &pbtInitData, &szInitDataLen);

  while (nfc_initiator_select_passive_target(pnd, nm, pbtInitData, szInitDataLen, &nt) > 0) {
    size_t i;
    bool seen = false;
    // Check if we've already seen this tag
    for (i = 0; i < szTargetFound; i++) {
      if (memcmp(&(ant[i]), &nt, sizeof(nfc_target)) == 0) {
        seen = true;
      }
    }
    if (seen) {
      break;
    }
    memcpy(&(ant[szTargetFound]), &nt, sizeof(nfc_target));
    szTargetFound++;
    if (szTargets == szTargetFound) {
      break;
    }
    nfc_initiator_deselect_target(pnd);
    // deselect has no effect on FeliCa and Jewel cards so we'll stop after one...
    // ISO/IEC 14443 B' cards are polled at 100% probability so it's not possible to detect correctly two cards at the same time
    if ((nm.nmt == NMT_FELICA) || (nm.nmt == NMT_JEWEL) || (nm.nmt == NMT_ISO14443BI) || (nm.nmt == NMT_ISO14443B2SR) || (nm.nmt == NMT_ISO14443B2CT)) {
      break;
    }
  }
  if (bInfiniteSelect) {
    if ((res = nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, true)) < 0) {
      return res;
    }
  }
  return szTargetFound;
}

/** @ingroup initiator
 * @brief Polling for NFC targets
 * @return Returns polled targets count, otherwise returns libnfc's error code (negative value).
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pnmModulations desired modulations
 * @param szModulations size of \a pnmModulations
 * @param uiPollNr specifies the number of polling (0x01 – 0xFE: 1 up to 254 polling, 0xFF: Endless polling)
 * @note one polling is a polling for each desired target type
 * @param uiPeriod indicates the polling period in units of 150 ms (0x01 – 0x0F: 150ms – 2.25s)
 * @note e.g. if uiPeriod=10, it will poll each desired target type during 1.5s
 * @param[out] pnt pointer on \a nfc_target (over)writable struct
 */
int
nfc_initiator_poll_target(nfc_device *pnd,
                          const nfc_modulation *pnmModulations, const size_t szModulations,
                          const uint8_t uiPollNr, const uint8_t uiPeriod,
                          nfc_target *pnt)
{
  HAL(initiator_poll_target, pnd, pnmModulations, szModulations, uiPollNr, uiPeriod, pnt);
}


/** @ingroup initiator
 * @brief Select a target and request active or passive mode for D.E.P. (Data Exchange Protocol)
 * @return Returns selected D.E.P targets count on success, otherwise returns libnfc's error code (negative value).
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param ndm desired D.E.P. mode (\a NDM_ACTIVE or \a NDM_PASSIVE for active, respectively passive mode)
 * @param nbr desired baud rate
 * @param ndiInitiator pointer \a nfc_dep_info struct that contains \e NFCID3 and \e General \e Bytes to set to the initiator device (optionnal, can be \e NULL)
 * @param[out] pnt is a \a nfc_target struct pointer where target information will be put.
 * @param timeout in milliseconds
 *
 * The NFC device will try to find an available D.E.P. target. The standards
 * (ISO18092 and ECMA-340) describe the modulation that can be used for reader
 * to passive communications.
 *
 * @note \a nfc_dep_info will be returned when the target was acquired successfully.
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_initiator_select_dep_target(nfc_device *pnd,
                                const nfc_dep_mode ndm, const nfc_baud_rate nbr,
                                const nfc_dep_info *pndiInitiator, nfc_target *pnt, const int timeout)
{
  HAL(initiator_select_dep_target, pnd, ndm, nbr, pndiInitiator, pnt, timeout);
}

/** @ingroup initiator
 * @brief Poll a target and request active or passive mode for D.E.P. (Data Exchange Protocol)
 * @return Returns selected D.E.P targets count on success, otherwise returns libnfc's error code (negative value).
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param ndm desired D.E.P. mode (\a NDM_ACTIVE or \a NDM_PASSIVE for active, respectively passive mode)
 * @param nbr desired baud rate
 * @param ndiInitiator pointer \a nfc_dep_info struct that contains \e NFCID3 and \e General \e Bytes to set to the initiator device (optionnal, can be \e NULL)
 * @param[out] pnt is a \a nfc_target struct pointer where target information will be put.
 * @param timeout in milliseconds
 *
 * The NFC device will try to find an available D.E.P. target. The standards
 * (ISO18092 and ECMA-340) describe the modulation that can be used for reader
 * to passive communications.
 *
 * @note \a nfc_dep_info will be returned when the target was acquired successfully.
 */
int
nfc_initiator_poll_dep_target(struct nfc_device *pnd,
                              const nfc_dep_mode ndm, const nfc_baud_rate nbr,
                              const nfc_dep_info *pndiInitiator,
                              nfc_target *pnt,
                              const int timeout)
{
  const int period = 300;
  int remaining_time = timeout;
  int res;
  int result = 0;
  bool bInfiniteSelect = pnd->bInfiniteSelect;
  if ((res = nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, true)) < 0)
    return res;
  while (remaining_time > 0) {
    if ((res = nfc_initiator_select_dep_target(pnd, ndm, nbr, pndiInitiator, pnt, period)) < 0) {
      if (res != NFC_ETIMEOUT) {
        result = res;
        goto end;
      }
    }
    if (res == 1) {
      result = res;
      goto end;
    }
    remaining_time -= period;
  }
end:
  if (! bInfiniteSelect) {
    if ((res = nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false)) < 0) {
      return res;
    }
  }
  return result;
}

/** @ingroup initiator
 * @brief Deselect a selected passive or emulated tag
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value).
 * @param pnd \a nfc_device struct pointer that represents currently used device
 *
 * After selecting and communicating with a passive tag, this function could be
 * used to deactivate and release the tag. This is very useful when there are
 * multiple tags available in the field. It is possible to use the \fn
 * nfc_initiator_select_passive_target() function to select the first available
 * tag, test it for the available features and support, deselect it and skip to
 * the next tag until the correct tag is found.
 */
int
nfc_initiator_deselect_target(nfc_device *pnd)
{
  HAL(initiator_deselect_target, pnd);
}

/** @ingroup initiator
 * @brief Send data to target then retrieve data from target
 * @return Returns received bytes count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represents currently used device
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTx contains the length in bytes.
 * @param[out] pbtRx response from the target
 * @param szRx size of \a pbtRx (Will return NFC_EOVFLOW if RX exceeds this size)
 * @param timeout in milliseconds
 *
 * The NFC device (configured as initiator) will transmit the supplied bytes (\a pbtTx) to the target.
 * It waits for the response and stores the received bytes in the \a pbtRx byte array.
 *
 * If \a NP_EASY_FRAMING option is disabled the frames will sent and received in raw mode: \e PN53x will not handle input neither output data.
 *
 * The parity bits are handled by the \e PN53x chip. The CRC can be generated automatically or handled manually.
 * Using this function, frames can be communicated very fast via the NFC initiator to the tag.
 *
 * Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 *
 * @warning The configuration option \a NP_HANDLE_PARITY must be set to \c true (the default value).
 *
 * @note When used with MIFARE Classic, NFC_EMFCAUTHFAIL error is returned if authentication command failed. You need to re-select the tag to operate with.
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_initiator_transceive_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx,
                               const size_t szRx, int timeout)
{
  HAL(initiator_transceive_bytes, pnd, pbtTx, szTx, pbtRx, szRx, timeout)
}

/** @ingroup initiator
 * @brief Transceive raw bit-frames to a target
 * @return Returns received bits count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represents currently used device
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTxBits contains the length in bits.
 *
 * @note For example the REQA (0x26) command (first anti-collision command of
 * ISO14443-A) must be precise 7 bits long. This is not possible by using
 * nfc_initiator_transceive_bytes(). With that function you can only
 * communicate frames that consist of full bytes. When you send a full byte (8
 * bits + 1 parity) with the value of REQA (0x26), a tag will simply not
 * respond. More information about this can be found in the anti-collision
 * example (\e nfc-anticol).
 *
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 *
 * @note For example if you send the SELECT_ALL (0x93, 0x20) = [ 10010011,
 * 00100000 ] command, you have to supply the following parity bytes (0x01,
 * 0x00) to define the correct odd parity bits. This is only an example to
 * explain how it works, if you just are sending two bytes with ISO14443-A
 * compliant parity bits you better can use the
 * nfc_initiator_transceive_bytes() function.
 *
 * @param[out] pbtRx response from the target
 * @param szRx size of \a pbtRx (Will return NFC_EOVFLOW if RX exceeds this size)
 * @param[out] pbtRxPar parameter contains a byte array of the corresponding parity bits
 *
 * The NFC device (configured as \e initiator) will transmit low-level messages
 * where only the modulation is handled by the \e PN53x chip. Construction of
 * the frame (data, CRC and parity) is completely done by libnfc.  This can be
 * very useful for testing purposes. Some protocols (e.g. MIFARE Classic)
 * require to violate the ISO14443-A standard by sending incorrect parity and
 * CRC bytes. Using this feature you are able to simulate these frames.
 */
int
nfc_initiator_transceive_bits(nfc_device *pnd,
                              const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar,
                              uint8_t *pbtRx, const size_t szRx,
                              uint8_t *pbtRxPar)
{
  (void)szRx;
  HAL(initiator_transceive_bits, pnd, pbtTx, szTxBits, pbtTxPar, pbtRx, pbtRxPar);
}

/** @ingroup initiator
 * @brief Send data to target then retrieve data from target
 * @return Returns received bytes count on success, otherwise returns libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represents currently used device
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTx contains the length in bytes.
 * @param[out] pbtRx response from the target
 * @param szRx size of \a pbtRx (Will return NFC_EOVFLOW if RX exceeds this size)
 *
 * This function is similar to nfc_initiator_transceive_bytes() with the following differences:
 * - A precise cycles counter will indicate the number of cycles between emission & reception of frames.
 * - It only supports mode with \a NP_EASY_FRAMING option disabled.
 * - Overall communication with the host is heavier and slower.
 *
 * Timer control:
 * By default timer configuration tries to maximize the precision, which also limits the maximum
 * cycles count before saturation/timeout.
 * E.g. with PN53x it can count up to 65535 cycles, so about 4.8ms, with a precision of about 73ns.
 * - If you're ok with the defaults, set *cycles = 0 before calling this function.
 * - If you need to count more cycles, set *cycles to the maximum you expect but don't forget
 *   you'll loose in precision and it'll take more time before timeout, so don't abuse!
 *
 * @warning The configuration option \a NP_EASY_FRAMING must be set to \c false.
 * @warning The configuration option \a NP_HANDLE_PARITY must be set to \c true (the default value).
 */
int
nfc_initiator_transceive_bytes_timed(nfc_device *pnd,
                                     const uint8_t *pbtTx, const size_t szTx,
                                     uint8_t *pbtRx, const size_t szRx,
                                     uint32_t *cycles)
{
  HAL(initiator_transceive_bytes_timed, pnd, pbtTx, szTx, pbtRx, szRx, cycles);
}

/** @ingroup initiator
 * @brief Check target presence
 * @return Returns 0 on success, otherwise returns libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pnt a \a nfc_target struct pointer where desired target information was stored (optionnal, can be \e NULL).
 * This function tests if \a nfc_target (or last selected tag if \e NULL) is currently present on NFC device.
 * @warning The target have to be selected before check its presence
 * @warning To run the test, one or more commands will be sent to target
*/
int
nfc_initiator_target_is_present(nfc_device *pnd, const nfc_target *pnt)
{
  HAL(initiator_target_is_present, pnd, pnt);
}

/** @ingroup initiator
 * @brief Transceive raw bit-frames to a target
 * @return Returns received bits count on success, otherwise returns libnfc's error code
 *
 * This function is similar to nfc_initiator_transceive_bits() with the following differences:
 * - A precise cycles counter will indicate the number of cycles between emission & reception of frames.
 * - It only supports mode with \a NP_EASY_FRAMING option disabled and CRC must be handled manually.
 * - Overall communication with the host is heavier and slower.
 *
 * Timer control:
 * By default timer configuration tries to maximize the precision, which also limits the maximum
 * cycles count before saturation/timeout.
 * E.g. with PN53x it can count up to 65535 cycles, so about 4.8ms, with a precision of about 73ns.
 * - If you're ok with the defaults, set *cycles = 0 before calling this function.
 * - If you need to count more cycles, set *cycles to the maximum you expect but don't forget
 *   you'll loose in precision and it'll take more time before timeout, so don't abuse!
 *
 * @warning The configuration option \a NP_EASY_FRAMING must be set to \c false.
 * @warning The configuration option \a NP_HANDLE_CRC must be set to \c false.
 * @warning The configuration option \a NP_HANDLE_PARITY must be set to \c true (the default value).
 */
int
nfc_initiator_transceive_bits_timed(nfc_device *pnd,
                                    const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar,
                                    uint8_t *pbtRx, const size_t szRx,
                                    uint8_t *pbtRxPar,
                                    uint32_t *cycles)
{
  (void)szRx;
  HAL(initiator_transceive_bits_timed, pnd, pbtTx, szTxBits, pbtTxPar, pbtRx, pbtRxPar, cycles);
}

/** @ingroup target
 * @brief Initialize NFC device as an emulated tag
 * @return Returns received bytes count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pnt pointer to \a nfc_target struct that represents the wanted emulated target
 *
 * @note \a pnt can be updated by this function: if you set NBR_UNDEFINED
 * and/or NDM_UNDEFINED (ie. for DEP mode), these fields will be updated.
 *
 * @param[out] pbtRx Rx buffer pointer
 * @param[out] szRx received bytes count
 * @param timeout in milliseconds
 *
 * This function initializes NFC device in \e target mode in order to emulate a
 * tag using the specified \a nfc_target_mode_t.
 * - Crc is handled by the device (NP_HANDLE_CRC = true)
 * - Parity is handled the device (NP_HANDLE_PARITY = true)
 * - Cryto1 cipher is disabled (NP_ACTIVATE_CRYPTO1 = false)
 * - Auto-switching in ISO14443-4 mode is enabled (NP_AUTO_ISO14443_4 = true)
 * - Easy framing is disabled (NP_EASY_FRAMING = false)
 * - Invalid frames are not accepted (NP_ACCEPT_INVALID_FRAMES = false)
 * - Multiple frames are not accepted (NP_ACCEPT_MULTIPLE_FRAMES = false)
 * - RF field is dropped
 *
 * @warning Be aware that this function will wait (hang) until a command is
 * received that is not part of the anti-collision. The RATS command for
 * example would wake up the emulator. After this is received, the send and
 * receive functions can be used.
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_target_init(nfc_device *pnd, nfc_target *pnt, uint8_t *pbtRx, const size_t szRx, int timeout)
{
  int res = 0;
  // Disallow invalid frame
  if ((res = nfc_device_set_property_bool(pnd, NP_ACCEPT_INVALID_FRAMES, false)) < 0)
    return res;
  // Disallow multiple frames
  if ((res = nfc_device_set_property_bool(pnd, NP_ACCEPT_MULTIPLE_FRAMES, false)) < 0)
    return res;
  // Make sure we reset the CRC and parity to chip handling.
  if ((res = nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true)) < 0)
    return res;
  if ((res = nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, true)) < 0)
    return res;
  // Activate auto ISO14443-4 switching by default
  if ((res = nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, true)) < 0)
    return res;
  // Activate "easy framing" feature by default
  if ((res = nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true)) < 0)
    return res;
  // Deactivate the CRYPTO1 cipher, it may could cause problems when still active
  if ((res = nfc_device_set_property_bool(pnd, NP_ACTIVATE_CRYPTO1, false)) < 0)
    return res;
  // Drop explicitely the field
  if ((res = nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false)) < 0)
    return res;

  HAL(target_init, pnd, pnt, pbtRx, szRx, timeout);
}

/** @ingroup dev
 * @brief Turn NFC device in idle mode
 * @return Returns 0 on success, otherwise returns libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 *
 * This function switch the device in idle mode.
 * In initiator mode, the RF field is turned off and the device is set to low power mode (if avaible);
 * In target mode, the emulation is stoped (no target available from external initiator) and the device is set to low power mode (if avaible).
 */
int
nfc_idle(nfc_device *pnd)
{
  HAL(idle, pnd);
}

/** @ingroup dev
 * @brief Abort current running command
 * @return Returns 0 on success, otherwise returns libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 *
 * Some commands (ie. nfc_target_init()) are blocking functions and will return only in particular conditions (ie. external initiator request).
 * This function attempt to abort the current running command.
 *
 * @note The blocking function (ie. nfc_target_init()) will failed with DEABORT error.
 */
int
nfc_abort_command(nfc_device *pnd)
{
  HAL(abort_command, pnd);
}

/** @ingroup target
 * @brief Send bytes and APDU frames
 * @return Returns sent bytes count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pbtTx pointer to Tx buffer
 * @param szTx size of Tx buffer
 * @param timeout in milliseconds
 *
 * This function make the NFC device (configured as \e target) send byte frames
 * (e.g. APDU responses) to the \e initiator.
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_target_send_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, int timeout)
{
  HAL(target_send_bytes, pnd, pbtTx, szTx, timeout);
}

/** @ingroup target
 * @brief Receive bytes and APDU frames
 * @return Returns received bytes count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pbtRx pointer to Rx buffer
 * @param szRx size of Rx buffer
 * @param timeout in milliseconds
 *
 * This function retrieves bytes frames (e.g. ADPU) sent by the \e initiator to the NFC device (configured as \e target).
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_target_receive_bytes(nfc_device *pnd, uint8_t *pbtRx, const size_t szRx, int timeout)
{
  HAL(target_receive_bytes, pnd, pbtRx, szRx, timeout);
}

/** @ingroup target
 * @brief Send raw bit-frames
 * @return Returns sent bits count on success, otherwise returns libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pbtTx pointer to Tx buffer
 * @param szTxBits size of Tx buffer
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 * This function can be used to transmit (raw) bit-frames to the \e initiator
 * using the specified NFC device (configured as \e target).
 */
int
nfc_target_send_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar)
{
  HAL(target_send_bits, pnd, pbtTx, szTxBits, pbtTxPar);
}

/** @ingroup target
 * @brief Receive bit-frames
 * @return Returns received bits count on success, otherwise returns libnfc's error code
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pbtRx pointer to Rx buffer
 * @param szRx size of Rx buffer
 * @param[out] pbtRxPar parameter contains a byte array of the corresponding parity bits
 *
 * This function makes it possible to receive (raw) bit-frames.  It returns all
 * the messages that are stored in the FIFO buffer of the \e PN53x chip.  It
 * does not require to send any frame and thereby could be used to snoop frames
 * that are transmitted by a nearby \e initiator.  @note Check out the
 * NP_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted
 * frames.
 */
int
nfc_target_receive_bits(nfc_device *pnd, uint8_t *pbtRx, const size_t szRx, uint8_t *pbtRxPar)
{
  HAL(target_receive_bits, pnd, pbtRx, szRx, pbtRxPar);
}

static struct sErrorMessage {
  int     iErrorCode;
  const char *pcErrorMsg;
} sErrorMessages[] = {
  /* Chip-level errors (internal errors, RF errors, etc.) */
  { NFC_SUCCESS, "Success" },
  { NFC_EIO, "Input / Output Error" },
  { NFC_EINVARG, "Invalid argument(s)" },
  { NFC_EDEVNOTSUPP, "Not Supported by Device" },
  { NFC_ENOTSUCHDEV, "No Such Device" },
  { NFC_EOVFLOW, "Buffer Overflow" },
  { NFC_ETIMEOUT, "Timeout" },
  { NFC_EOPABORTED, "Operation Aborted" },
  { NFC_ENOTIMPL, "Not (yet) Implemented" },
  { NFC_ETGRELEASED, "Target Released" },
  { NFC_EMFCAUTHFAIL, "Mifare Authentication Failed" },
  { NFC_ERFTRANS, "RF Transmission Error" },
  { NFC_ECHIP, "Device's Internal Chip Error" },
};

/** @ingroup error
 * @brief Return the last error string
 * @return Returns a string
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 */
const char *
nfc_strerror(const nfc_device *pnd)
{
  const char *pcRes = "Unknown error";
  size_t  i;
  for (i = 0; i < (sizeof(sErrorMessages) / sizeof(struct sErrorMessage)); i++) {
    if (sErrorMessages[i].iErrorCode == pnd->last_error) {
      pcRes = sErrorMessages[i].pcErrorMsg;
      break;
    }
  }

  return pcRes;
}

/** @ingroup error
 * @brief Renders the last error in pcStrErrBuf for a maximum size of szBufLen chars
 * @return Returns 0 upon success
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pcStrErrBuf a string that contains the last error.
 * @param szBufLen size of buffer
 */
int
nfc_strerror_r(const nfc_device *pnd, char *pcStrErrBuf, size_t szBufLen)
{
  return (snprintf(pcStrErrBuf, szBufLen, "%s", nfc_strerror(pnd)) < 0) ? -1 : 0;
}

/** @ingroup error
 * @brief Display the last error occured on a nfc_device
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param pcString a string
 */
void
nfc_perror(const nfc_device *pnd, const char *pcString)
{
  fprintf(stderr, "%s: %s\n", pcString, nfc_strerror(pnd));
}

/** @ingroup error
 * @brief Returns last error occured on a nfc_device
 * @return Returns an integer that represents to libnfc's error code.
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 */
int
nfc_device_get_last_error(const nfc_device *pnd)
{
  return pnd->last_error;
}

/* Special data accessors */

/** @ingroup data
 * @brief Returns the device name
 * @return Returns a string with the device name
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 */
const char *
nfc_device_get_name(nfc_device *pnd)
{
  return pnd->name;
}

/** @ingroup data
 * @brief Returns the device connection string
 * @return Returns a string with the device connstring
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 */
const char *
nfc_device_get_connstring(nfc_device *pnd)
{
  return pnd->connstring;
}

/** @ingroup data
 * @brief Get supported modulations.
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param mode \a nfc_mode.
 * @param supported_mt pointer of \a nfc_modulation_type array.
 *
 */
int
nfc_device_get_supported_modulation(nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type **const supported_mt)
{
  HAL(get_supported_modulation, pnd, mode, supported_mt);
}

/** @ingroup data
 * @brief Get supported baud rates.
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param nmt \a nfc_modulation_type.
 * @param supported_br pointer of \a nfc_baud_rate array.
 *
 */
int
nfc_device_get_supported_baud_rate(nfc_device *pnd, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br)
{
  HAL(get_supported_baud_rate, pnd, nmt, supported_br);
}

/* Misc. functions */

/** @ingroup misc
 * @brief Returns the library version
 * @return Returns a string with the library version
 *
 * @param pnd \a nfc_device struct pointer that represent currently used device
 */
const char *
nfc_version(void)
{
#ifdef GIT_REVISION
  return GIT_REVISION;
#else
  return PACKAGE_VERSION;
#endif // GIT_REVISION
}

/** @ingroup misc
 * @brief Free buffer allocated by libnfc
 *
 * @param pointer on buffer that needs to be freed
 */
void
nfc_free(void *p)
{
  free(p);
}

/** @ingroup misc
 * @brief Print information about NFC device
 * @return Upon successful return, this function returns the number of characters printed (excluding the null byte used to end output to strings), otherwise returns libnfc's error code (negative value)
 * @param pnd \a nfc_device struct pointer that represent currently used device
 * @param buf pointer where string will be allocated, then information printed
 *
 * @warning *buf must be freed using nfc_free()
 */
int
nfc_device_get_information_about(nfc_device *pnd, char **buf)
{
  HAL(device_get_information_about, pnd, buf);
}

/** @ingroup string-converter
 * @brief Convert \a nfc_baud_rate value to string
 * @return Returns nfc baud rate
 * @param \a nfc_baud_rate to convert
*/
const char *
str_nfc_baud_rate(const nfc_baud_rate nbr)
{
  switch (nbr) {
    case NBR_UNDEFINED:
      return "undefined baud rate";
      break;
    case NBR_106:
      return "106 kbps";
      break;
    case NBR_212:
      return "212 kbps";
      break;
    case NBR_424:
      return "424 kbps";
      break;
    case NBR_847:
      return "847 kbps";
      break;
  }
  // Should never go there..
  return "";
}

/** @ingroup string-converter
 * @brief Convert \a nfc_modulation_type value to string
 * @return Returns nfc modulation type
 * @param \a nfc_modulation_type to convert
*/
const char *
str_nfc_modulation_type(const nfc_modulation_type nmt)
{
  switch (nmt) {
    case NMT_ISO14443A:
      return "ISO/IEC 14443A";
      break;
    case NMT_ISO14443B:
      return "ISO/IEC 14443-4B";
      break;
    case NMT_ISO14443BI:
      return "ISO/IEC 14443-4B'";
      break;
    case NMT_ISO14443B2CT:
      return "ISO/IEC 14443-2B ASK CTx";
      break;
    case NMT_ISO14443B2SR:
      return "ISO/IEC 14443-2B ST SRx";
      break;
    case NMT_FELICA:
      return "FeliCa";
      break;
    case NMT_JEWEL:
      return "Innovision Jewel";
      break;
    case NMT_DEP:
      return "D.E.P.";
      break;
  }
  // Should never go there..
  return "";
}

/** @ingroup string-converter
 * @brief Convert \a nfc_modulation_type value to string
 * @return Upon successful return, this function returns the number of characters printed (excluding the null byte used to end output to strings), otherwise returns libnfc's error code (negative value)
 * @param nt \a nfc_target struct to print
 * @param buf pointer where string will be allocated, then nfc target information printed
 *
 * @warning *buf must be freed using nfc_free()
*/
int
str_nfc_target(char **buf, const nfc_target *pnt, bool verbose)
{
  *buf = malloc(4096);
  if (! *buf)
    return NFC_ESOFT;
  (*buf)[0] = '\0';
  snprint_nfc_target(*buf, 4096, pnt, verbose);
  return strlen(*buf);
}
