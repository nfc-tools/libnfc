/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * @file acr122.c
 * @brief Driver for ACR122 devices (e.g. Tikitag, Touchatag, ACS ACR122)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#include "chips/pn53x.h"
#include "drivers/acr122.h"
#include "nfc-internal.h"

// Bus
#include <winscard.h>

#  define ACR122_DRIVER_NAME "ACR122"

#if defined (_WIN32)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(3500)
#elif defined(__APPLE__)
#  include <wintypes.h>
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

#define ACR122_WRAP_LEN 5
#define ACR122_COMMAND_LEN 266
#define ACR122_RESPONSE_LEN 268

const struct pn53x_io acr122_io;

char   *acr122_firmware (nfc_device_t *pnd);

const char *supported_devices[] = {
  "ACS ACR122",         // ACR122U & Touchatag, last version
  "ACS ACR 38U-CCID",   // Touchatag, early version
  "ACS ACR38U-CCID",    // Touchatag, early version, under MacOSX
  "    CCID USB",       // ??
  NULL
};

struct acr122_data {
  SCARDHANDLE hCard;
  SCARD_IO_REQUEST ioCard;
  byte_t  abtRx[ACR122_RESPONSE_LEN];
  size_t  szRx;
};

#define DRIVER_DATA(pnd) ((struct acr122_data*)(pnd->driver_data))

static SCARDCONTEXT _SCardContext;
static int _iSCardContextRefCount = 0;

SCARDCONTEXT *
acr122_get_scardcontext (void)
{
  if (_iSCardContextRefCount == 0) {
    if (SCardEstablishContext (SCARD_SCOPE_USER, NULL, NULL, &_SCardContext) != SCARD_S_SUCCESS)
      return NULL;
  }
  _iSCardContextRefCount++;

  return &_SCardContext;
}

void
acr122_free_scardcontext (void)
{
  if (_iSCardContextRefCount) {
    _iSCardContextRefCount--;
    if (!_iSCardContextRefCount) {
      SCardReleaseContext (_SCardContext);
    }
  }
}

#define PCSC_MAX_DEVICES 16
/**
 * @brief List connected devices
 *
 * Probe PCSC to find NFC capable hardware.
 *
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 * @return true if succeeded, false otherwise.
 */
bool
acr122_probe (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
{
  size_t  szPos = 0;
  char    acDeviceNames[256 + 64 * PCSC_MAX_DEVICES];
  size_t  szDeviceNamesLen = sizeof (acDeviceNames);
  uint32_t uiBusIndex = 0;
  SCARDCONTEXT *pscc;
  bool    bSupported;
  int     i;

  // Clear the reader list
  memset (acDeviceNames, '\0', szDeviceNamesLen);

  *pszDeviceFound = 0;

  // Test if context succeeded
  if (!(pscc = acr122_get_scardcontext ())) {
    DBG ("%s", "PCSC context not found");
    return false;
  }
  // Retrieve the string array of all available pcsc readers
  DWORD dwDeviceNamesLen = szDeviceNamesLen;
  if (SCardListReaders (*pscc, NULL, acDeviceNames, &dwDeviceNamesLen) != SCARD_S_SUCCESS)
    return false;

  // DBG("%s", "PCSC reports following device(s):");

  while ((acDeviceNames[szPos] != '\0') && ((*pszDeviceFound) < szDevices)) {
    uiBusIndex++;

    // DBG("- %s (pos=%ld)", acDeviceNames + szPos, (unsigned long) szPos);

    bSupported = false;
    for (i = 0; supported_devices[i] && !bSupported; i++) {
      int     l = strlen (supported_devices[i]);
      bSupported = 0 == strncmp (supported_devices[i], acDeviceNames + szPos, l);
    }

    if (bSupported) {
      // Supported ACR122 device found
      strncpy (pnddDevices[*pszDeviceFound].acDevice, acDeviceNames + szPos, DEVICE_NAME_LENGTH - 1);
      pnddDevices[*pszDeviceFound].pcDriver = ACR122_DRIVER_NAME;
      pnddDevices[*pszDeviceFound].uiBusIndex = uiBusIndex;
      (*pszDeviceFound)++;
    } else {
      DBG ("PCSC device [%s] is not NFC capable or not supported by libnfc.", acDeviceNames + szPos);
    }

    // Find next device name position
    while (acDeviceNames[szPos++] != '\0');
  }
  acr122_free_scardcontext ();

  return true;
}

nfc_device_t *
acr122_connect (const nfc_device_desc_t * pndd)
{
  char   *pcFirmware;
  nfc_device_t *pnd = nfc_device_new ();
  pnd->driver_data = malloc (sizeof (struct acr122_data));
  pnd->chip_data = malloc (sizeof (struct pn53x_data));

  SCARDCONTEXT *pscc;

  DBG ("Attempt to connect to %s", pndd->acDevice);
  // Test if context succeeded
  if (!(pscc = acr122_get_scardcontext ()))
    goto error;
  // Test if we were able to connect to the "emulator" card
  if (SCardConnect (*pscc, pndd->acDevice, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &(DRIVER_DATA (pnd)->hCard), (void *) &(DRIVER_DATA (pnd)->ioCard.dwProtocol)) != SCARD_S_SUCCESS) {
    // Connect to ACR122 firmware version >2.0
    if (SCardConnect (*pscc, pndd->acDevice, SCARD_SHARE_DIRECT, 0, &(DRIVER_DATA (pnd)->hCard), (void *) &(DRIVER_DATA (pnd)->ioCard.dwProtocol)) != SCARD_S_SUCCESS) {
      // We can not connect to this device.
      DBG ("%s", "PCSC connect failed");
      goto error;
    }
  }
  // Configure I/O settings for card communication
  DRIVER_DATA (pnd)->ioCard.cbPciLength = sizeof (SCARD_IO_REQUEST);

  // Retrieve the current firmware version
  pcFirmware = acr122_firmware (pnd);
  if (strstr (pcFirmware, FIRMWARE_TEXT) != NULL) {

    // Done, we found the reader we are looking for
    snprintf (pnd->acName, sizeof (pnd->acName), "%s / %s", pndd->acDevice, pcFirmware);

    CHIP_DATA (pnd)->power_mode = NORMAL;
    CHIP_DATA (pnd)->io = &acr122_io;
    // 50: empirical tuning on Touchatag
    // 46: empirical tuning on ACR122U
    CHIP_DATA (pnd)->timer_correction = 50;

    pnd->driver = &acr122_driver;

    pn53x_init (pnd);

    return pnd;
  }

error:
  nfc_device_free (pnd);

  return NULL;
}

void
acr122_disconnect (nfc_device_t * pnd)
{
  SCardDisconnect (DRIVER_DATA (pnd)->hCard, SCARD_LEAVE_CARD);
  acr122_free_scardcontext ();

  nfc_device_free (pnd);
}

bool
acr122_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData)
{
  // Make sure the command does not overflow the send buffer
  if (szData > ACR122_COMMAND_LEN) {
    pnd->iLastError = DEIO;
    return false;
  }

  // Prepare and transmit the send buffer
  const size_t szTxBuf = szData + 6;
  byte_t  abtTxBuf[ACR122_WRAP_LEN + ACR122_COMMAND_LEN] = { 0xFF, 0x00, 0x00, 0x00, szData + 1, 0xD4 };
  memcpy (abtTxBuf + 6, pbtData, szData);
#ifdef DEBUG
  PRINT_HEX ("TX", abtTxBuf, szTxBuf);
#endif

  DRIVER_DATA (pnd)->szRx = 0;

  DWORD dwRxLen = sizeof (DRIVER_DATA (pnd)->abtRx);

  if (DRIVER_DATA (pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
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
    if (SCardControl (DRIVER_DATA (pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtTxBuf, szTxBuf, DRIVER_DATA (pnd)->abtRx, ACR122_RESPONSE_LEN, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->iLastError = DEIO;
      return false;
    }
  } else {
    /*
     * In T=0 mode, we receive an acknoledge from the MCU, in T=1 mode, we
     * receive the response from the PN532.
     */
    if (SCardTransmit (DRIVER_DATA (pnd)->hCard, &(DRIVER_DATA (pnd)->ioCard), abtTxBuf, szTxBuf, NULL, DRIVER_DATA (pnd)->abtRx, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->iLastError = DEIO;
      return false;
    }
  }

  if (DRIVER_DATA (pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_T0) {
   /*
    * Check the MCU response
    */

    // Make sure we received the byte-count we expected
    if (dwRxLen != 2) {
      pnd->iLastError = DEIO;
      return false;
    }
    // Check if the operation was successful, so an answer is available
    if (DRIVER_DATA (pnd)->abtRx[0] == SCARD_OPERATION_ERROR) {
      pnd->iLastError = DEISERRFRAME;
      return false;
    }
  } else {
    DRIVER_DATA (pnd)->szRx = dwRxLen;
  }

  return true;
}

int
acr122_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szData)
{
  int len;
  byte_t  abtRxCmd[5] = { 0xFF, 0xC0, 0x00, 0x00 };

  if (DRIVER_DATA (pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_T0) {
    /*
     * Retrieve the PN532 response.
     */
    DWORD dwRxLen = sizeof (DRIVER_DATA (pnd)->abtRx);
    abtRxCmd[4] = DRIVER_DATA (pnd)->abtRx[1];
    if (SCardTransmit (DRIVER_DATA (pnd)->hCard, &(DRIVER_DATA (pnd)->ioCard), abtRxCmd, sizeof (abtRxCmd), NULL, DRIVER_DATA (pnd)->abtRx, &dwRxLen) != SCARD_S_SUCCESS) {
      pnd->iLastError = DEIO;
      return -1;
    }
    DRIVER_DATA (pnd)->szRx = dwRxLen;
  } else {
    /*
     * We already have the PN532 answer, it was saved by acr122_send().
     */
  }
#ifdef DEBUG
  PRINT_HEX ("RX", DRIVER_DATA (pnd)->abtRx, DRIVER_DATA (pnd)->szRx);
#endif

  // Make sure we have an emulated answer that fits the return buffer
  if (DRIVER_DATA (pnd)->szRx < 4 || (DRIVER_DATA (pnd)->szRx - 4) > szData) {
    pnd->iLastError = DEIO;
    return -1;
  }
  // Wipe out the 4 APDU emulation bytes: D5 4B .. .. .. 90 00
  len = DRIVER_DATA (pnd)->szRx - 4;
  memcpy (pbtData, DRIVER_DATA (pnd)->abtRx + 2, len);

  // Transmission went successful
  return len;
}

char   *
acr122_firmware (nfc_device_t *pnd)
{
  byte_t  abtGetFw[5] = { 0xFF, 0x00, 0x48, 0x00, 0x00 };
  uint32_t uiResult;

  static char abtFw[11];
  DWORD dwFwLen = sizeof (abtFw);
  memset (abtFw, 0x00, sizeof (abtFw));
  if (DRIVER_DATA (pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
    uiResult = SCardControl (DRIVER_DATA (pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtGetFw, sizeof (abtGetFw), (byte_t *) abtFw, dwFwLen-1, &dwFwLen);
  } else {
    uiResult = SCardTransmit (DRIVER_DATA (pnd)->hCard, &(DRIVER_DATA (pnd)->ioCard), abtGetFw, sizeof (abtGetFw), NULL, (byte_t *) abtFw, &dwFwLen);
  }

  if (uiResult != SCARD_S_SUCCESS) {
    ERR ("No ACR122 firmware received, Error: %08x", uiResult);
  }

  return abtFw;
}

#if 0
bool
acr122_led_red (nfc_device_t *pnd, bool bOn)
{
  byte_t  abtLed[9] = { 0xFF, 0x00, 0x40, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00 };
  byte_t  abtBuf[2];
  DWORD dwBufLen = sizeof (abtBuf);
  (void) bOn;
  if (DRIVER_DATA (pnd)->ioCard.dwProtocol == SCARD_PROTOCOL_UNDEFINED) {
    return (SCardControl (DRIVER_DATA (pnd)->hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, abtLed, sizeof (abtLed), abtBuf, dwBufLen, &dwBufLen) == SCARD_S_SUCCESS);
  } else {
    return (SCardTransmit (DRIVER_DATA (pnd)->hCard, &(DRIVER_DATA (pnd)->ioCard), abtLed, sizeof (abtLed), NULL, abtBuf, &dwBufLen) == SCARD_S_SUCCESS);
  }
}
#endif

const struct pn53x_io acr122_io = {
  .send    = acr122_send,
  .receive = acr122_receive,
};

const struct nfc_driver_t acr122_driver = {
  .name       = ACR122_DRIVER_NAME,
  .probe      = acr122_probe,
  .connect    = acr122_connect,
  .disconnect = acr122_disconnect,
  .strerror   = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
  .initiator_poll_targets           = NULL,
  .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
  .initiator_deselect_target        = pn53x_initiator_deselect_target,
  .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
  .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
  .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
  .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,

  .target_init           = NULL,
  .target_send_bytes     = NULL,
  .target_receive_bytes  = NULL,
  .target_send_bits      = NULL,
  .target_receive_bits   = NULL,

  .configure  = pn53x_configure,
};

