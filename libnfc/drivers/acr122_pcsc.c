/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2011, Romain Tartiere, Romuald Conty
 * Copyright (C) 2012, Romuald Conty
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
 * @file acr122_pcsc.c
 * @brief Driver for ACR122 devices (e.g. Tikitag, Touchatag, ACS ACR122) behind PC/SC
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#include "chips/pn53x.h"
#include "drivers/acr122_pcsc.h"
#include "nfc-internal.h"

// Bus
#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

#define ACR122_PCSC_DRIVER_NAME "acr122_pcsc"

#if defined (_WIN32)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(3500)
#elif defined(__APPLE__)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE (((0x31) << 16) | ((3500) << 2))
#elif defined (__FreeBSD__) || defined (__OpenBSD__)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE (((0x31) << 16) | ((3500) << 2))
#elif defined (__linux__)
#  include <reader.h>
// Escape IOCTL tested successfully:
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(1)
#else
#    error "Can't determine serial string for your system"
#endif

#include <nfc/nfc.h>

#define SCARD_OPERATION_SUCCESS	0x61
#define SCARD_OPERATION_ERROR	0x63

#ifndef SCARD_PROTOCOL_UNDEFINED
#  define SCARD_PROTOCOL_UNDEFINED SCARD_PROTOCOL_UNSET
#endif

#define FIRMWARE_TEXT "ACR122U" // Tested on: ACR122U101(ACS), ACR122U102(Tikitag), ACR122U203(ACS)

#define ACR122_PCSC_WRAP_LEN 5
#define ACR122_PCSC_COMMAND_LEN 266
#define ACR122_PCSC_RESPONSE_LEN 268

#define LOG_CATEGORY "libnfc.driver.acr122"

const struct pn53x_io acr122_pcsc_io;

char   *acr122_pcsc_firmware(nfc_device *pnd);

const char *supported_devices[] = {
  "ACS ACR122",         // ACR122U & Touchatag, last version
  "ACS ACR 38U-CCID",   // Touchatag, early version
  "ACS ACR38U-CCID",    // Touchatag, early version, under MacOSX
  "ACS AET65",          // Touchatag using CCID driver version >= 1.4.6
  "    CCID USB",       // ??
  NULL
};

struct acr122_pcsc_data {
  SCARDHANDLE hCard;
  SCARD_IO_REQUEST ioCard;
  uint8_t  abtRx[ACR122_PCSC_RESPONSE_LEN];
  size_t  szRx;
};

#define DRIVER_DATA(pnd) ((struct acr122_pcsc_data*)(pnd->driver_data))

static SCARDCONTEXT _SCardContext;
static int _iSCardContextRefCount = 0;

static SCARDCONTEXT *
acr122_pcsc_get_scardcontext(void)
{
  if (_iSCardContextRefCount == 0) {
    if (SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &_SCardContext) != SCARD_S_SUCCESS)
      return NULL;
  }
  _iSCardContextRefCount++;

  return &_SCardContext;
}

static void
acr122_pcsc_free_scardcontext(void)
{
  if (_iSCardContextRefCount) {
    _iSCardContextRefCount--;
    if (!_iSCardContextRefCount) {
      SCardReleaseContext(_SCardContext);
    }
  }
}

#define PCSC_MAX_DEVICES 16
/**
 * @brief List opened devices
 *
 * Probe PCSC to find NFC capable hardware.
 *
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 * @return true if succeeded, false otherwise.
 */
bool
acr122_pcsc_probe(nfc_connstring connstrings[], size_t connstrings_len, size_t *pszDeviceFound)
{
  size_t  szPos = 0;
  char    acDeviceNames[256 + 64 * PCSC_MAX_DEVICES];
  size_t  szDeviceNamesLen = sizeof(acDeviceNames);
  SCARDCONTEXT *pscc;
  bool    bSupported;
  int     i;

  // Clear the reader list
  memset(acDeviceNames, '\0', szDeviceNamesLen);

  *pszDeviceFound = 0;

  // Test if context succeeded
  if (!(pscc = acr122_pcsc_get_scardcontext())) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_WARN, "%s", "PCSC context not found (make sure PCSC daemon is running).");
    return false;
  }
  // Retrieve the string array of all available pcsc readers
  DWORD dwDeviceNamesLen = szDeviceNamesLen;
  if (SCardListReaders(*pscc, NULL, acDeviceNames, &dwDeviceNamesLen) != SCARD_S_SUCCESS)
    return false;

  while ((acDeviceNames[szPos] != '\0') && ((*pszDeviceFound) < connstrings_len)) {

    // DBG("- %s (pos=%ld)", acDeviceNames + szPos, (unsigned long) szPos);

    bSupported = false;
    for (i = 0; supported_devices[i] && !bSupported; i++) {
      int     l = strlen(supported_devices[i]);
      bSupported = 0 == strncmp(supported_devices[i], acDeviceNames + szPos, l);
    }

    if (bSupported) {
      // Supported ACR122 device found
      snprintf(connstrings[*pszDeviceFound], sizeof(nfc_connstring), "%s:%s", ACR122_PCSC_DRIVER_NAME, acDeviceNames + szPos);
      (*pszDeviceFound)++;
    } else {
      log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "PCSC device [%s] is not NFC capable or not supported by libnfc.", acDeviceNames + szPos);
    }

    // Find next device name position
    while (acDeviceNames[szPos++] != '\0');
  }
  acr122_pcsc_free_scardcontext();

  return true;
}

struct acr122_pcsc_descriptor {
  char pcsc_device_name[512];
};

static int
acr122_pcsc_connstring_decode(const nfc_connstring connstring, struct acr122_pcsc_descriptor *desc)
{
  char *cs = malloc(strlen(connstring) + 1);
  if (!cs) {
    perror("malloc");
    return -1;
  }
  strcpy(cs, connstring);
  const char *driver_name = strtok(cs, ":");
  if (!driver_name) {
    // Parse error
    free(cs);
    return -1;
  }

  if (0 != strcmp(driver_name, ACR122_PCSC_DRIVER_NAME)) {
    // Driver name does not match.
    free(cs);
    return 0;
  }

  const char *device_name = strtok(NULL, ":");
  if (!device_name) {
    // Only driver name was specified (or parsing error)
    free(cs);
    return 1;
  }
  strncpy(desc->pcsc_device_name, device_name, sizeof(desc->pcsc_device_name) - 1);
  desc->pcsc_device_name[sizeof(desc->pcsc_device_name) - 1] = '\0';

  free(cs);
  return 2;

  free(cs);
  return 3;
}

nfc_device *
acr122_pcsc_open(const nfc_connstring connstring)
{
  struct acr122_pcsc_descriptor ndd;
  int connstring_decode_level = acr122_pcsc_connstring_decode(connstring, &ndd);

  if (connstring_decode_level < 1) {
    return NULL;
  }

  nfc_connstring fullconnstring;
  if (connstring_decode_level == 1) {
    // Device was not specified, take the first one we can find
    size_t szDeviceFound;
    acr122_pcsc_probe(&fullconnstring, 1, &szDeviceFound);
    if (szDeviceFound < 1)
      return NULL;
    connstring_decode_level = acr122_pcsc_connstring_decode(fullconnstring, &ndd);
    if (connstring_decode_level < 2) {
      return NULL;
    }
  } else {
    memcpy(fullconnstring, connstring, sizeof(nfc_connstring));
  }
  if (strlen(ndd.pcsc_device_name) < 5) { // We can assume it's a reader ID as pcsc_name always ends with "NN NN"
    // Device was not specified, only ID, retrieve it
    size_t index;
    if (sscanf(ndd.pcsc_device_name, "%lu", &index) != 1)
      return NULL;
    nfc_connstring *ncs = malloc(sizeof(nfc_connstring) * (index + 1));
    size_t szDeviceFound;
    acr122_pcsc_probe(ncs, index + 1, &szDeviceFound);
    if (szDeviceFound < index + 1)
      return NULL;
    strncpy(fullconnstring, ncs[index], sizeof(nfc_connstring));
    free(ncs);
    connstring_decode_level = acr122_pcsc_connstring_decode(fullconnstring, &ndd);
    if (connstring_decode_level < 2) {
      return NULL;
    }
  }

  char   *pcFirmware;
  nfc_device *pnd = nfc_device_new(fullconnstring);
  pnd->driver_data = malloc(sizeof(struct acr122_pcsc_data));

  // Alloc and init chip's data
  pn53x_data_new(pnd, &acr122_pcsc_io);

  SCARDCONTEXT *pscc;

  log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "Attempt to open %s", ndd.pcsc_device_name);
  // Test if context succeeded
  if (!(pscc = acr122_pcsc_get_scardcontext()))
    goto error;
  // Test if we were able to connect to the "emulator" card
  if (SCardConnect(*pscc, ndd.pcsc_device_name, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &(DRIVER_DATA(pnd)->hCard), (void *) & (DRIVER_DATA(pnd)->ioCard.dwProtocol)) != SCARD_S_SUCCESS) {
    // Connect to ACR122 firmware version >2.0
    if (SCardConnect(*pscc, ndd.pcsc_device_name, SCARD_SHARE_DIRECT, 0, &(DRIVER_DATA(pnd)->hCard), (void *) & (DRIVER_DATA(pnd)->ioCard.dwProtocol)) != SCARD_S_SUCCESS) {
      // We can not connect to this device.
      log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "%s", "PCSC connect failed");
      goto error;
    }
  }
  // Configure I/O settings for card communication
  DRIVER_DATA(pnd)->ioCard.cbPciLength = sizeof(SCARD_IO_REQUEST);

  // Retrieve the current firmware version
  pcFirmware = acr122_pcsc_firmware(pnd);
  if (strstr(pcFirmware, FIRMWARE_TEXT) != NULL) {

    // Done, we found the reader we are looking for
    snprintf(pnd->name, sizeof(pnd->name), "%s / %s", ndd.pcsc_device_name, pcFirmware);

    // 50: empirical tuning on Touchatag
    // 46: empirical tuning on ACR122U
    CHIP_DATA(pnd)->timer_correction = 50;

    pnd->driver = &acr122_pcsc_driver;

    pn53x_init(pnd);

    return pnd;
  }

error:
  nfc_device_free(pnd);

  return NULL;
}

void
acr122_pcsc_close(nfc_device *pnd)
{
  SCardDisconnect(DRIVER_DATA(pnd)->hCard, SCARD_LEAVE_CARD);
  acr122_pcsc_free_scardcontext();

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

int
acr122_pcsc_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout)
{
  // FIXME: timeout is not handled
  (void) timeout;

  // Make sure the command does not overflow the send buffer
  if (szData > ACR122_PCSC_COMMAND_LEN) {
    pnd->last_error = NFC_EINVARG;
    return pnd->last_error;
  }

  // Prepare and transmit the send buffer
  const size_t szTxBuf = szData + 6;
  uint8_t  abtTxBuf[ACR122_PCSC_WRAP_LEN + ACR122_PCSC_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00, szData + 1, 0xD4 };
  memcpy(abtTxBuf + 6, pbtData, szData);
  LOG_HEX("TX", abtTxBuf, szTxBuf);

  DRIVER_DATA(pnd)->szRx = 0;

  DWORD dwRxLen = sizeof(DRIVER_DATA(pnd)->abtRx);

  if (DRIVER_DATA(pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
    /*
     * In this communication mode, we directly have the response from the
     * PN532.  Save it in the driver data structure so that it can be retrieved
     * in ac122_receive().
     *
     * Some devices will never enter this state (e.g. Touchatag) but are still
     * supported through SCardTransmit calls (see bellow).
     *
     * This state is generaly reached when the ACR122 has no target in it's
     * field.
     */
    if (SCardControl(DRIVER_DATA(pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtTxBuf, szTxBuf, DRIVER_DATA(pnd)->abtRx, ACR122_PCSC_RESPONSE_LEN, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
  } else {
    /*
     * In T=0 mode, we receive an acknoledge from the MCU, in T=1 mode, we
     * receive the response from the PN532.
     */
    if (SCardTransmit(DRIVER_DATA(pnd)->hCard, &(DRIVER_DATA(pnd)->ioCard), abtTxBuf, szTxBuf, NULL, DRIVER_DATA(pnd)->abtRx, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
  }

  if (DRIVER_DATA(pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_T0) {
    /*
     * Check the MCU response
     */

    // Make sure we received the byte-count we expected
    if (dwRxLen != 2) {
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
    // Check if the operation was successful, so an answer is available
    if (DRIVER_DATA(pnd)->abtRx[0] == SCARD_OPERATION_ERROR) {
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
  } else {
    DRIVER_DATA(pnd)->szRx = dwRxLen;
  }

  return NFC_SUCCESS;
}

int
acr122_pcsc_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szData, int timeout)
{
  // FIXME: timeout is not handled
  (void) timeout;

  int len;
  uint8_t  abtRxCmd[5] = { 0xFF, 0xC0, 0x00, 0x00 };

  if (DRIVER_DATA(pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_T0) {
    /*
     * Retrieve the PN532 response.
     */
    DWORD dwRxLen = sizeof(DRIVER_DATA(pnd)->abtRx);
    abtRxCmd[4] = DRIVER_DATA(pnd)->abtRx[1];
    if (SCardTransmit(DRIVER_DATA(pnd)->hCard, &(DRIVER_DATA(pnd)->ioCard), abtRxCmd, sizeof(abtRxCmd), NULL, DRIVER_DATA(pnd)->abtRx, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->last_error = NFC_EIO;
      return pnd->last_error;
    }
    DRIVER_DATA(pnd)->szRx = dwRxLen;
  } else {
    /*
     * We already have the PN532 answer, it was saved by acr122_pcsc_send().
     */
  }
  LOG_HEX("RX", DRIVER_DATA(pnd)->abtRx, DRIVER_DATA(pnd)->szRx);

  // Make sure we have an emulated answer that fits the return buffer
  if (DRIVER_DATA(pnd)->szRx < 4 || (DRIVER_DATA(pnd)->szRx - 4) > szData) {
    pnd->last_error = NFC_EIO;
    return pnd->last_error;
  }
  // Wipe out the 4 APDU emulation bytes: D5 4B .. .. .. 90 00
  len = DRIVER_DATA(pnd)->szRx - 4;
  memcpy(pbtData, DRIVER_DATA(pnd)->abtRx + 2, len);

  return len;
}

char   *
acr122_pcsc_firmware(nfc_device *pnd)
{
  uint8_t  abtGetFw[5] = { 0xFF, 0x00, 0x48, 0x00, 0x00 };
  uint32_t uiResult;

  static char abtFw[11];
  DWORD dwFwLen = sizeof(abtFw);
  memset(abtFw, 0x00, sizeof(abtFw));
  if (DRIVER_DATA(pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
    uiResult = SCardControl(DRIVER_DATA(pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtGetFw, sizeof(abtGetFw), (uint8_t *) abtFw, dwFwLen - 1, &dwFwLen);
  } else {
    uiResult = SCardTransmit(DRIVER_DATA(pnd)->hCard, &(DRIVER_DATA(pnd)->ioCard), abtGetFw, sizeof(abtGetFw), NULL, (uint8_t *) abtFw, &dwFwLen);
  }

  if (uiResult != SCARD_S_SUCCESS) {
    log_put(LOG_CATEGORY, NFC_PRIORITY_ERROR, "No ACR122 firmware received, Error: %08x", uiResult);
  }

  return abtFw;
}

#if 0
bool
acr122_pcsc_led_red(nfc_device *pnd, bool bOn)
{
  uint8_t  abtLed[9] = { 0xFF, 0x00, 0x40, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00 };
  uint8_t  abtBuf[2];
  DWORD dwBufLen = sizeof(abtBuf);
  (void) bOn;
  if (DRIVER_DATA(pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
    return (SCardControl(DRIVER_DATA(pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtLed, sizeof(abtLed), abtBuf, dwBufLen, &dwBufLen) == SCARD_S_SUCCESS);
  } else {
    return (SCardTransmit(DRIVER_DATA(pnd)->hCard, &(DRIVER_DATA(pnd)->ioCard), abtLed, sizeof(abtLed), NULL, abtBuf, &dwBufLen) == SCARD_S_SUCCESS);
  }
}
#endif

const struct pn53x_io acr122_pcsc_io = {
  .send    = acr122_pcsc_send,
  .receive = acr122_pcsc_receive,
};

const struct nfc_driver acr122_pcsc_driver = {
  .name                             = ACR122_PCSC_DRIVER_NAME,
  .probe                            = acr122_pcsc_probe,
  .open                             = acr122_pcsc_open,
  .close                            = acr122_pcsc_close,
  .strerror                         = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_init_secure_element    = NULL, // No secure-element support
  .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
  .initiator_poll_target            = pn53x_initiator_poll_target,
  .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
  .initiator_deselect_target        = pn53x_initiator_deselect_target,
  .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
  .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
  .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
  .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,
  .initiator_target_is_present      = pn53x_initiator_target_is_present,

  .target_init           = pn53x_target_init,
  .target_send_bytes     = pn53x_target_send_bytes,
  .target_receive_bytes  = pn53x_target_receive_bytes,
  .target_send_bits      = pn53x_target_send_bits,
  .target_receive_bits   = pn53x_target_receive_bits,

  .device_set_property_bool     = pn53x_set_property_bool,
  .device_set_property_int      = pn53x_set_property_int,
  .get_supported_modulation     = pn53x_get_supported_modulation,
  .get_supported_baud_rate      = pn53x_get_supported_baud_rate,
  .device_get_information_about = pn53x_get_information_about,

  .abort_command  = NULL,  // Abort is not supported in this driver
  .idle  = NULL,           // Idle is not supported in this driver
};

