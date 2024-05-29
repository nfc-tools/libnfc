/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2019      Frank Morgner
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2020      Feitian Technologies
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
 * @file pcsc.c
 * @brief Driver for non-ACR122 devices behind PC/SC
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <nfc/nfc.h>

#include "drivers/pcsc.h"
#include "nfc-internal.h"

// Bus
#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
// define from pcsclite for apple
#define SCARD_AUTOALLOCATE (DWORD)(-1)

#define SCARD_ATTR_VALUE(Class, Tag) ((((ULONG)(Class)) << 16) | ((ULONG)(Tag)))

#define SCARD_CLASS_VENDOR_INFO     1   /**< Vendor information definitions */
#define SCARD_CLASS_COMMUNICATIONS  2   /**< Communication definitions */
#define SCARD_CLASS_PROTOCOL        3   /**< Protocol definitions */
#define SCARD_CLASS_POWER_MGMT      4   /**< Power Management definitions */
#define SCARD_CLASS_SECURITY        5   /**< Security Assurance definitions */
#define SCARD_CLASS_MECHANICAL      6   /**< Mechanical characteristic definitions */
#define SCARD_CLASS_VENDOR_DEFINED  7   /**< Vendor specific definitions */
#define SCARD_CLASS_IFD_PROTOCOL    8   /**< Interface Device Protocol options */
#define SCARD_CLASS_ICC_STATE       9   /**< ICC State specific definitions */
#define SCARD_CLASS_SYSTEM     0x7fff   /**< System-specific definitions */

#define SCARD_ATTR_VENDOR_NAME SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0100) /**< Vendor name. */
#define SCARD_ATTR_VENDOR_IFD_TYPE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0101) /**< Vendor-supplied interface device type (model designation of reader). */
#define SCARD_ATTR_VENDOR_IFD_VERSION SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0102) /**< Vendor-supplied interface device version (DWORD in the form 0xMMmmbbbb where MM = major version, mm = minor version, and bbbb = build number). */
#define SCARD_ATTR_VENDOR_IFD_SERIAL_NO SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0103) /**< Vendor-supplied interface device serial number. */
#define SCARD_ATTR_ICC_TYPE_PER_ATR SCARD_ATTR_VALUE(SCARD_CLASS_ICC_STATE, 0x0304) /**< Single byte indicating smart card type */
#else
#ifndef _Win32
#include <reader.h>
#endif
#include <winscard.h>
#endif

#ifdef WIN32
#include <windows.h>
#define usleep(x) Sleep((x + 999) / 1000)
#endif

#define PCSC_DRIVER_NAME "pcsc"

#include <nfc/nfc.h>

#define LOG_GROUP    NFC_LOG_GROUP_DRIVER
#define LOG_CATEGORY "libnfc.driver.pcsc"

static const char *supported_devices[] = {
  "ACS ACR122",         // ACR122U & Touchatag, last version
  "ACS ACR 38U-CCID",   // Touchatag, early version
  "ACS ACR38U-CCID",    // Touchatag, early version, under MacOSX
  "ACS AET65",          // Touchatag using CCID driver version >= 1.4.6
  "    CCID USB",       // ??
  NULL
};

struct pcsc_data {
  SCARDHANDLE hCard;
  SCARD_IO_REQUEST ioCard;
  DWORD dwShareMode;
  DWORD last_error;
};

#define DRIVER_DATA(pnd) ((struct pcsc_data*)(pnd->driver_data))

static SCARDCONTEXT _SCardContext;
static int _iSCardContextRefCount = 0;

const nfc_baud_rate pcsc_supported_brs[] = {NBR_106, NBR_424, 0};
const nfc_modulation_type pcsc_supported_mts[] = {NMT_ISO14443A, NMT_ISO14443B, 0};

static SCARDCONTEXT *
pcsc_get_scardcontext(void)
{
  if (_iSCardContextRefCount == 0) {
    if (SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &_SCardContext) != SCARD_S_SUCCESS)
      return NULL;
  }
  _iSCardContextRefCount++;

  return &_SCardContext;
}

static void
pcsc_free_scardcontext(void)
{
  if (_iSCardContextRefCount) {
    _iSCardContextRefCount--;
    if (!_iSCardContextRefCount) {
      SCardReleaseContext(_SCardContext);
    }
  }
}

#define ICC_TYPE_UNKNOWN 0
#define ICC_TYPE_14443A  5
#define ICC_TYPE_14443B  6

bool is_pcsc_reader_vendor_feitian(const struct nfc_device *pnd);

static int pcsc_transmit(struct nfc_device *pnd, const uint8_t *tx, const size_t tx_len, uint8_t *rx, size_t *rx_len)
{
  struct pcsc_data *data = pnd->driver_data;
  DWORD dw_rx_len = *rx_len;
  //in libfreefare, tx_len = 1, and it leads to 0x80100008 error, with PC/SC reader, the input tx_len at least two bytes for the SW value
  //so if found the reader is Feitian reader, we set to 2
  if (is_pcsc_reader_vendor_feitian(pnd))
  {
    if (dw_rx_len == 1)
    {
      dw_rx_len = 2;
    } else {
      dw_rx_len += 2;//in libfreefare, some data length send not include sw1 and sw2, so add it.
    }
  }

  LOG_HEX(NFC_LOG_GROUP_COM, "TX", tx, tx_len);

  data->last_error = SCardTransmit(data->hCard, &data->ioCard, tx, tx_len,
                                   NULL, rx, &dw_rx_len);
  if (data->last_error != SCARD_S_SUCCESS) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "PCSC transmit failed");
    return NFC_EIO;
  }
  *rx_len = dw_rx_len;

  LOG_HEX(NFC_LOG_GROUP_COM, "RX", rx, *rx_len);

  return NFC_SUCCESS;
}

static int pcsc_get_status(struct nfc_device *pnd, int *target_present, uint8_t *atr, size_t *atr_len)
{
  struct pcsc_data *data = pnd->driver_data;
  DWORD dw_atr_len = *atr_len, reader_len, state, protocol;

  data->last_error = SCardStatus(data->hCard, NULL, &reader_len, &state, &protocol, atr, &dw_atr_len);
  if (data->last_error != SCARD_S_SUCCESS
      && data->last_error != SCARD_W_RESET_CARD
      && data->last_error != SCARD_W_REMOVED_CARD) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Get status failed");
    return NFC_EIO;
  }

  *target_present = state & SCARD_PRESENT;
  *atr_len = dw_atr_len;

  return NFC_SUCCESS;
}

static int pcsc_reconnect(struct nfc_device *pnd, DWORD share_mode, DWORD protocol, DWORD disposition)
{
  struct pcsc_data *data = pnd->driver_data;

  data->last_error = SCardReconnect(data->hCard, share_mode, protocol, disposition, &data->ioCard.dwProtocol);
  if (data->last_error != SCARD_S_SUCCESS
      && data->last_error != SCARD_W_RESET_CARD
      && data->last_error != SCARD_E_NO_SMARTCARD) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Reconnect failed");
    return NFC_EIO;
  }

  data->dwShareMode = share_mode;

  return NFC_SUCCESS;
}

static uint8_t pcsc_get_icc_type(const struct nfc_device *pnd)
{
  struct pcsc_data *data = pnd->driver_data;
  uint8_t it = 0;
  DWORD dwItLen = sizeof it;
  data->last_error = SCardGetAttrib(data->hCard, SCARD_ATTR_ICC_TYPE_PER_ATR, &it, &dwItLen);
  return it;
}

static bool is_pcsc_reader_vendor(const struct nfc_device *pnd, const char *target_vendor_name)
{
  bool isTarget = false;
  if (pnd == NULL || strlen(pnd->name) == 0) {
    return isTarget;
  }

  return  isTarget = (strstr(pnd->name, target_vendor_name)) ? true : false;
}

bool is_pcsc_reader_vendor_feitian(const struct nfc_device *pnd)
{
  return is_pcsc_reader_vendor(pnd, "Feitian") || is_pcsc_reader_vendor(pnd, "FeiTian") || is_pcsc_reader_vendor(pnd, "feitian") || is_pcsc_reader_vendor(pnd, "FEITIAN");
}

//get atqa by send apdu
static int pcsc_get_atqa(struct nfc_device *pnd, uint8_t *atqa, size_t atqa_len)
{
  const uint8_t get_data[] = {0xFF, 0xCA, 0x03, 0x00, 0x00};
  uint8_t resp[256 + 2];
  size_t resp_len = sizeof resp;

  pnd->last_error = pcsc_transmit(pnd, get_data, sizeof get_data, resp, &resp_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (resp_len < 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Reader doesn't support request for ATQA");
    pnd->last_error = NFC_EDEVNOTSUPP;
    return pnd->last_error;
  }
  if (atqa_len < resp_len - 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "ATQA length is wrong");
    pnd->last_error = NFC_ESOFT;
    return pnd->last_error;
  }

  memcpy(atqa, resp, resp_len - 2);
  return resp_len - 2;
}

//get ats by send apdu
static int pcsc_get_ats(struct nfc_device *pnd, uint8_t *ats, size_t ats_len)
{
  const uint8_t get_data[] = {0xFF, 0xCA, 0x01, 0x00, 0x00};
  uint8_t resp[256 + 2];
  size_t resp_len = sizeof resp;

  pnd->last_error = pcsc_transmit(pnd, get_data, sizeof get_data, resp, &resp_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (resp_len <= 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Reader doesn't support request for ATS");
    pnd->last_error = NFC_EDEVNOTSUPP;
    return pnd->last_error;
  }
  if (ats_len < resp_len - 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "ATS length is wrong");
    pnd->last_error = NFC_ESOFT;
    return pnd->last_error;
  }

  memcpy(ats, resp + 1, resp_len - 2 - 1);//data expect TL and SW1SW2
  return resp_len - 2 - 1;
}

//get sak by send apdu
static int pcsc_get_sak(struct nfc_device *pnd, uint8_t *sak, size_t sak_len)
{
  const uint8_t get_data[] = {0xFF, 0xCA, 0x02, 0x00, 0x00};
  uint8_t resp[256 + 2];
  size_t resp_len = sizeof resp;

  pnd->last_error = pcsc_transmit(pnd, get_data, sizeof get_data, resp, &resp_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (resp_len < 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Reader doesn't support request for SAK");
    pnd->last_error = NFC_EDEVNOTSUPP;
    return pnd->last_error;
  }
  if (sak_len < resp_len - 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "SAK length is wrong");
    pnd->last_error = NFC_ESOFT;
    return pnd->last_error;
  }

  memcpy(sak, resp, resp_len - 2);
  return resp_len - 2;
}

static int pcsc_get_uid(struct nfc_device *pnd, uint8_t *uid, size_t uid_len)
{
  const uint8_t get_data[] = {0xFF, 0xCA, 0x00, 0x00, 0x00};
  uint8_t resp[256 + 2];
  size_t resp_len = sizeof resp;

  pnd->last_error = pcsc_transmit(pnd, get_data, sizeof get_data, resp, &resp_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (resp_len < 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Reader doesn't support request for UID");
    pnd->last_error = NFC_EDEVNOTSUPP;
    return pnd->last_error;
  }
  if (uid_len < resp_len - 2) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "UID too big");
    pnd->last_error = NFC_ESOFT;
    return pnd->last_error;
  }

  memcpy(uid, resp, resp_len - 2);
  return resp_len - 2;
}

static int pcsc_props_to_target(struct nfc_device *pnd, uint8_t it, const uint8_t *patr, size_t szatr, const uint8_t *puid, int szuid, const nfc_modulation_type nmt, nfc_target *pnt)
{
  if (NULL != pnt) {
    switch (nmt) {
      case NMT_ISO14443A:
        if ((it == ICC_TYPE_UNKNOWN || it == ICC_TYPE_14443A)
            && (szuid <= 0 || szuid == 4 || szuid == 7 || szuid == 10)
            && NULL != patr && szatr >= 5
            && patr[0] == 0x3B
            && patr[1] == (0x80 | ((uint8_t)(szatr - 5)))
            && patr[2] == 0x80
            && patr[3] == 0x01) {
          memset(pnt, 0, sizeof * pnt);
          pnt->nm.nmt = NMT_ISO14443A;
          pnt->nm.nbr = pcsc_supported_brs[0];
          if (szuid > 0) {
            memcpy(pnt->nti.nai.abtUid, puid, szuid);
            pnt->nti.nai.szUidLen = szuid;
          }
          if (is_pcsc_reader_vendor_feitian(pnd)) {
            uint8_t atqa[2];
            pcsc_get_atqa(pnd, atqa, sizeof(atqa));
            //ATQA Coding of NXP Contactless Card ICs
            if(atqa[0] == 0x00 || atqa[0] == 0x03)
            {
              memcpy(pnt->nti.nai.abtAtqa,atqa,2);
            }else {
              pnt->nti.nai.abtAtqa[0] = atqa[1];
              pnt->nti.nai.abtAtqa[1] = atqa[0];
            }

            uint8_t sak[1];
            pcsc_get_sak(pnd, sak, sizeof(sak));
            pnt->nti.nai.btSak = sak[0];
            uint8_t ats[256];
            int ats_len = pcsc_get_ats(pnd, ats, sizeof(ats));
            ats_len = (ats_len > 0 ? ats_len : 0);//The reader may not support to get ATS
            memcpy(pnt->nti.nai.abtAts, ats, ats_len);
            pnt->nti.nai.szAtsLen = ats_len;
          } else {
            /* SAK_ISO14443_4_COMPLIANT */
            pnt->nti.nai.btSak = 0x20;
            /* Choose TL, TA, TB, TC according to Mifare DESFire */
            memcpy(pnt->nti.nai.abtAts, "\x75\x77\x81\x02", 4);
            /* copy historical bytes */
            memcpy(pnt->nti.nai.abtAts + 4, patr + 4, (uint8_t)(szatr - 5));
            pnt->nti.nai.szAtsLen = 4 + (uint8_t)(szatr - 5);
          }

          return NFC_SUCCESS;
        }
        break;
      case NMT_ISO14443B:
        if ((ICC_TYPE_UNKNOWN == 0 || ICC_TYPE_14443B == 6)
            && (szuid <= 0 || szuid == 8)
            && NULL != patr && szatr == 5 + 8
            && patr[0] == 0x3B
            && patr[1] == (0x80 | 0x08)
            && patr[2] == 0x80
            && patr[3] == 0x01) {
          memset(pnt, 0, sizeof * pnt);
          pnt->nm.nmt = NMT_ISO14443B;
          pnt->nm.nbr = pcsc_supported_brs[0];
          memcpy(pnt->nti.nbi.abtApplicationData, patr + 4, 4);
          memcpy(pnt->nti.nbi.abtProtocolInfo, patr + 8, 3);
          /* PI_ISO14443_4_SUPPORTED */
          pnt->nti.nbi.abtProtocolInfo[1] = 0x01;
          return NFC_SUCCESS;
        }
        break;
      default:
        break;
    }
  }
  return NFC_EINVARG;
}

#define PCSC_MAX_DEVICES 16
/**
 * @brief List opened devices
 *
 * Probe PCSC to find any reader but the ACR122 devices (ACR122U and Touchatag/Tikitag).
 *
 * @param connstring array of nfc_connstring where found device's connection strings will be stored.
 * @param connstrings_len size of connstrings array.
 * @return number of devices found.
 */
static size_t
pcsc_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  (void) context;
  size_t  szPos = 0;
  char    acDeviceNames[256 + 64 * PCSC_MAX_DEVICES];
  size_t  szDeviceNamesLen = sizeof(acDeviceNames);
  SCARDCONTEXT *pscc;
  int     i;

  // Clear the reader list
  memset(acDeviceNames, '\0', szDeviceNamesLen);

  // Test if context succeeded
  if (!(pscc = pcsc_get_scardcontext())) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "Warning: %s", "PCSC context not found (make sure PCSC daemon is running).");
    return 0;
  }
  // Retrieve the string array of all available pcsc readers
  DWORD dwDeviceNamesLen = szDeviceNamesLen;
  if (SCardListReaders(*pscc, NULL, acDeviceNames, &dwDeviceNamesLen) != SCARD_S_SUCCESS)
    return 0;

  size_t device_found = 0;
  while ((acDeviceNames[szPos] != '\0') && (device_found < connstrings_len)) {
    bool bSupported = false;
    for (i = 0; supported_devices[i] && !bSupported; i++) {
      int     l = strlen(supported_devices[i]);
      bSupported = 0 == !strncmp(supported_devices[i], acDeviceNames + szPos, l);
    }

    if (bSupported) {
      // Supported non-ACR122 device found
      snprintf(connstrings[device_found], sizeof(nfc_connstring), "%s:%s", PCSC_DRIVER_NAME, acDeviceNames + szPos);
      device_found++;
    } else {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Skipping PCSC device [%s] as it is supported by acr122_pcsc driver.", acDeviceNames + szPos);
    }

    // Find next device name position
    while (acDeviceNames[szPos++] != '\0');
  }
  pcsc_free_scardcontext();

  return device_found;
}

struct pcsc_descriptor {
  char *pcsc_device_name;
};

static nfc_device *
pcsc_open(const nfc_context *context, const nfc_connstring connstring)
{
  struct pcsc_descriptor ndd;
  int connstring_decode_level = connstring_decode(connstring, PCSC_DRIVER_NAME, "pcsc", &ndd.pcsc_device_name, NULL);

  if (connstring_decode_level < 1) {
    return NULL;
  }

  nfc_connstring fullconnstring;
  if (connstring_decode_level == 1) {
    // Device was not specified, take the first one we can find
    size_t szDeviceFound = pcsc_scan(context, &fullconnstring, 1);
    if (szDeviceFound < 1)
      return NULL;
    connstring_decode_level = connstring_decode(fullconnstring, PCSC_DRIVER_NAME, "pcsc", &ndd.pcsc_device_name, NULL);
    if (connstring_decode_level < 2) {
      return NULL;
    }
  } else {
    memcpy(fullconnstring, connstring, sizeof(nfc_connstring));
  }
  if (strlen(ndd.pcsc_device_name) < 5) { // We can assume it's a reader ID as pcsc_name always ends with "NN NN"
    // Device was not specified, only ID, retrieve it
    size_t index;
    if (sscanf(ndd.pcsc_device_name, "%4" SCNuPTR, &index) != 1) {
      free(ndd.pcsc_device_name);
      return NULL;
    }
    nfc_connstring *ncs = malloc(sizeof(nfc_connstring) * (index + 1));
    if (!ncs) {
      perror("malloc");
      free(ndd.pcsc_device_name);
      return NULL;
    }
    size_t szDeviceFound = pcsc_scan(context, ncs, index + 1);
    if (szDeviceFound < index + 1) {
      free(ncs);
      free(ndd.pcsc_device_name);
      return NULL;
    }
    strncpy(fullconnstring, ncs[index], sizeof(nfc_connstring));
    fullconnstring[sizeof(nfc_connstring) - 1] = '\0';
    free(ncs);
    connstring_decode_level = connstring_decode(fullconnstring, PCSC_DRIVER_NAME, "pcsc", &ndd.pcsc_device_name, NULL);

    if (connstring_decode_level < 2) {
      free(ndd.pcsc_device_name);
      return NULL;
    }
  }

  nfc_device *pnd = nfc_device_new(context, fullconnstring);
  if (!pnd) {
    perror("malloc");
    goto error;
  }
  pnd->driver_data = malloc(sizeof(struct pcsc_data));
  if (!pnd->driver_data) {
    perror("malloc");
    goto error;
  }

  SCARDCONTEXT *pscc;

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Attempt to open %s", ndd.pcsc_device_name);
  // Test if context succeeded
  if (!(pscc = pcsc_get_scardcontext()))
    goto error;
  DRIVER_DATA(pnd)->last_error = SCardConnect(*pscc, ndd.pcsc_device_name, SCARD_SHARE_DIRECT, 0 | 1, &(DRIVER_DATA(pnd)->hCard), (void *) & (DRIVER_DATA(pnd)->ioCard.dwProtocol));
  if (DRIVER_DATA(pnd)->last_error != SCARD_S_SUCCESS) {
    // We can not connect to this device.
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "PCSC connect failed");
    goto error;
  }
  // Configure I/O settings for card communication
  DRIVER_DATA(pnd)->ioCard.cbPciLength = sizeof(SCARD_IO_REQUEST);
  DRIVER_DATA(pnd)->dwShareMode = SCARD_SHARE_DIRECT;

  // Done, we found the reader we are looking for
  snprintf(pnd->name, sizeof(pnd->name), "%s", ndd.pcsc_device_name);

  pnd->driver = &pcsc_driver;

  free(ndd.pcsc_device_name);
  return pnd;

error:
  free(ndd.pcsc_device_name);
  nfc_device_free(pnd);
  return NULL;
}

static void
pcsc_close(nfc_device *pnd)
{
  SCardDisconnect(DRIVER_DATA(pnd)->hCard, SCARD_LEAVE_CARD);
  pcsc_free_scardcontext();

  nfc_device_free(pnd);
}

static const char *stringify_error(const LONG pcscError)
{
  static char strError[75];
  const char *msg = NULL;

  switch (pcscError) {
    case SCARD_S_SUCCESS:
      msg = "Command successful.";
      break;
    case SCARD_F_INTERNAL_ERROR:
      msg = "Internal error.";
      break;
    case SCARD_E_CANCELLED:
      msg = "Command cancelled.";
      break;
    case SCARD_E_INVALID_HANDLE:
      msg = "Invalid handle.";
      break;
    case SCARD_E_INVALID_PARAMETER:
      msg = "Invalid parameter given.";
      break;
    case SCARD_E_INVALID_TARGET:
      msg = "Invalid target given.";
      break;
    case SCARD_E_NO_MEMORY:
      msg = "Not enough memory.";
      break;
    case SCARD_F_WAITED_TOO_LONG:
      msg = "Waited too long.";
      break;
    case SCARD_E_INSUFFICIENT_BUFFER:
      msg = "Insufficient buffer.";
      break;
    case SCARD_E_UNKNOWN_READER:
      msg = "Unknown reader specified.";
      break;
    case SCARD_E_TIMEOUT:
      msg = "Command timeout.";
      break;
    case SCARD_E_SHARING_VIOLATION:
      msg = "Sharing violation.";
      break;
    case SCARD_E_NO_SMARTCARD:
      msg = "No smart card inserted.";
      break;
    case SCARD_E_UNKNOWN_CARD:
      msg = "Unknown card.";
      break;
    case SCARD_E_CANT_DISPOSE:
      msg = "Cannot dispose handle.";
      break;
    case SCARD_E_PROTO_MISMATCH:
      msg = "Card protocol mismatch.";
      break;
    case SCARD_E_NOT_READY:
      msg = "Subsystem not ready.";
      break;
    case SCARD_E_INVALID_VALUE:
      msg = "Invalid value given.";
      break;
    case SCARD_E_SYSTEM_CANCELLED:
      msg = "System cancelled.";
      break;
    case SCARD_F_COMM_ERROR:
      msg = "RPC transport error.";
      break;
    case SCARD_F_UNKNOWN_ERROR:
      msg = "Unknown error.";
      break;
    case SCARD_E_INVALID_ATR:
      msg = "Invalid ATR.";
      break;
    case SCARD_E_NOT_TRANSACTED:
      msg = "Transaction failed.";
      break;
    case SCARD_E_READER_UNAVAILABLE:
      msg = "Reader is unavailable.";
      break;
    /* case SCARD_P_SHUTDOWN: */
    case SCARD_E_PCI_TOO_SMALL:
      msg = "PCI struct too small.";
      break;
    case SCARD_E_READER_UNSUPPORTED:
      msg = "Reader is unsupported.";
      break;
    case SCARD_E_DUPLICATE_READER:
      msg = "Reader already exists.";
      break;
    case SCARD_E_CARD_UNSUPPORTED:
      msg = "Card is unsupported.";
      break;
    case SCARD_E_NO_SERVICE:
      msg = "Service not available.";
      break;
    case SCARD_E_SERVICE_STOPPED:
      msg = "Service was stopped.";
      break;
    /* case SCARD_E_UNEXPECTED: */
    /* case SCARD_E_ICC_CREATEORDER: */
    /* case SCARD_E_UNSUPPORTED_FEATURE: */
    /* case SCARD_E_DIR_NOT_FOUND: */
    /* case SCARD_E_NO_DIR: */
    /* case SCARD_E_NO_FILE: */
    /* case SCARD_E_NO_ACCESS: */
    /* case SCARD_E_WRITE_TOO_MANY: */
    /* case SCARD_E_BAD_SEEK: */
    /* case SCARD_E_INVALID_CHV: */
    /* case SCARD_E_UNKNOWN_RES_MNG: */
    /* case SCARD_E_NO_SUCH_CERTIFICATE: */
    /* case SCARD_E_CERTIFICATE_UNAVAILABLE: */
    case SCARD_E_NO_READERS_AVAILABLE:
      msg = "Cannot find a smart card reader.";
      break;
    /* case SCARD_E_COMM_DATA_LOST: */
    /* case SCARD_E_NO_KEY_CONTAINER: */
    /* case SCARD_E_SERVER_TOO_BUSY: */
    case SCARD_W_UNSUPPORTED_CARD:
      msg = "Card is not supported.";
      break;
    case SCARD_W_UNRESPONSIVE_CARD:
      msg = "Card is unresponsive.";
      break;
    case SCARD_W_UNPOWERED_CARD:
      msg = "Card is unpowered.";
      break;
    case SCARD_W_RESET_CARD:
      msg = "Card was reset.";
      break;
    case SCARD_W_REMOVED_CARD:
      msg = "Card was removed.";
      break;
    /* case SCARD_W_SECURITY_VIOLATION: */
    /* case SCARD_W_WRONG_CHV: */
    /* case SCARD_W_CHV_BLOCKED: */
    /* case SCARD_W_EOF: */
    /* case SCARD_W_CANCELLED_BY_USER: */
    /* case SCARD_W_CARD_NOT_AUTHENTICATED: */

    case SCARD_E_UNSUPPORTED_FEATURE:
      msg = "Feature not supported.";
      break;
    default:
      (void)snprintf(strError, sizeof(strError) - 1, "Unknown error: 0x%08lX",
                     pcscError);
  };

  if (msg)
    (void)strncpy(strError, msg, sizeof(strError));
  else
    (void)snprintf(strError, sizeof(strError) - 1, "Unknown error: 0x%08lX",
                   pcscError);

  /* add a null byte */
  strError[sizeof(strError) - 1] = '\0';

  return strError;
}

static const char *
pcsc_strerror(const struct nfc_device *pnd)
{
  return stringify_error(DRIVER_DATA(pnd)->last_error);
}

static int pcsc_initiator_init(struct nfc_device *pnd)
{
  (void) pnd;
  return NFC_SUCCESS;
}

static int pcsc_initiator_select_passive_target(struct nfc_device *pnd,  const nfc_modulation nm, const uint8_t *pbtInitData, const size_t szInitData, nfc_target *pnt)
{
  uint8_t atr[MAX_ATR_SIZE];
  uint8_t uid[10];
  int target_present;
  size_t atr_len = sizeof atr;

  (void) pbtInitData;
  (void) szInitData;

  if (nm.nbr != pcsc_supported_brs[0] && nm.nbr != pcsc_supported_brs[1])
    return NFC_EINVARG;

  pnd->last_error = pcsc_get_status(pnd, &target_present, atr, &atr_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (!target_present) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "No target present");
    return NFC_ENOTSUCHDEV;
  }

  uint8_t icc_type = pcsc_get_icc_type(pnd);
  int uid_len = pcsc_get_uid(pnd, uid, sizeof uid);
  if (pcsc_props_to_target(pnd, icc_type, atr, atr_len, uid, uid_len, nm.nmt, pnt) != NFC_SUCCESS) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Type of target not supported");
    return NFC_EDEVNOTSUPP;
  }

  pnd->last_error = pcsc_reconnect(pnd, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  return 1;
}

#if 0
static int pcsc_initiator_deselect_target(struct nfc_device *pnd)
{
  pnd->last_error = pcsc_reconnect(pnd, SCARD_SHARE_DIRECT, 0, SCARD_LEAVE_CARD);
  return pnd->last_error;
}
#endif

static int pcsc_initiator_transceive_bytes(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, int timeout)
{
  size_t resp_len = szRx;

  // FIXME: timeout is not handled
  (void) timeout;

  if (is_pcsc_reader_vendor_feitian(pnd)) {
    LOG_HEX(NFC_LOG_GROUP_COM, "not feitian reader pcsc apdu send", pbtTx, szTx);

    uint8_t apdu_data[256];
    uint8_t resp[256 + 2];
    size_t send_size = 0;
    if (pbtTx[0] == 0x30) {//read data
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0xB0;
      apdu_data[2] = 0x00;
      apdu_data[3] = pbtTx[1];
      apdu_data[4] = 0x10;
      send_size = 5;
    } else if (pbtTx[0] == 0xA0 || pbtTx[0] == 0xA2) {//write data
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0xD6;
      apdu_data[2] = 0x00;
      apdu_data[3] = pbtTx[1];
      apdu_data[4] = szTx - 2;
      memcpy(apdu_data + 5, pbtTx + 2, szTx - 2);
      send_size = 5 + szTx - 2;
    } else if (pbtTx[0] == 0x60 || pbtTx[0] == 0x61 || pbtTx[0] == 0x1A) { //Auth command
      //load pin first
      {
        apdu_data[0] = 0xFF;
        apdu_data[1] = 0x82;
        apdu_data[2] = 0x00;
        apdu_data[3] = 0x01;
        apdu_data[4] = 0x06;
        memcpy(apdu_data + 5, pbtTx + 2, 6);
        send_size = 11;
        pnd->last_error = pcsc_transmit(pnd, apdu_data, send_size, resp, &resp_len);
        memset(apdu_data, 0, sizeof(apdu_data));
        memset(resp, 0, sizeof(resp));
        usleep(500000);//delay 500ms
      }
      // then auth
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0x86;
      apdu_data[2] = 0x00;
      apdu_data[3] = 0x00;
      apdu_data[4] = 0x05;
      apdu_data[5] = 0x01;
      apdu_data[6] = 0x00;
      apdu_data[7] = pbtTx[1];//block index
      apdu_data[8] = pbtTx[0];//type a or type b
      apdu_data[9] = 0x01;
      send_size = 10;
    } else if (pbtTx[0] == 0xC0) { //DECREMENT cmd
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0xD7;
      apdu_data[2] = 0x00;
      apdu_data[3] = pbtTx[1];//block index
      apdu_data[4] = 0x05;
      memcpy(apdu_data + 5, pbtTx + 2, szTx - 2);
      send_size = 5 + szTx - 2;
    } else if (pbtTx[0] == 0xC1) { //INCREMENT cmd
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0xD7;
      apdu_data[2] = 0x00;
      apdu_data[3] = pbtTx[1];//block index
      apdu_data[4] = 0x05;
      memcpy(apdu_data + 5, pbtTx + 2, szTx - 2);
      send_size = 5 + szTx - 2;
    } else if (pbtTx[0] == 0xC2) { //STORE cmd
      apdu_data[0] = 0xFF;
      apdu_data[1] = 0xD8;
      apdu_data[2] = 0x00;
      apdu_data[3] = pbtTx[1];
      apdu_data[4] = szTx - 2;
      memcpy(apdu_data + 5, pbtTx + 2, szTx - 2);
      send_size = 5 + szTx - 2;
    } else {//other cmd
      memcpy(apdu_data, pbtTx, szTx);
      send_size = szTx;
    }
    LOG_HEX(NFC_LOG_GROUP_COM, "feitian reader pcsc apdu send:", apdu_data, send_size);
    pnd->last_error = pcsc_transmit(pnd, apdu_data, send_size, resp, &resp_len);
    LOG_HEX(NFC_LOG_GROUP_COM, "feitian reader pcsc apdu received:", resp, resp_len);

    memcpy(pbtRx, resp, resp_len);
  } else {
    pnd->last_error = pcsc_transmit(pnd, pbtTx, szTx, pbtRx, &resp_len);
  }
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  return resp_len;
}

static int
pcsc_initiator_poll_target(struct nfc_device *pnd,
                           const nfc_modulation *pnmModulations, const size_t szModulations,
                           const uint8_t uiPollNr, const uint8_t uiPeriod,
                           nfc_target *pnt)
{
  static int periodFactor = 150000;
  int period = uiPeriod * periodFactor;

  if (pnd == NULL)
    return 0;

  for (int j = 0; j < uiPollNr; j++)
  {
    for (unsigned int i = 0; i < szModulations; i++)
    {
      const nfc_modulation nm = pnmModulations[i];

      nfc_target nt;
      int res = pcsc_initiator_select_passive_target(pnd, nm, 0, 0, &nt);
      if (res > 0 && pnt)
      {
        memcpy(pnt, &nt, sizeof(nfc_target));
        return res;
      }
    }
    usleep(period);
  }

  return 0;
}

static int pcsc_initiator_target_is_present(struct nfc_device *pnd, const nfc_target *pnt)
{
  uint8_t atr[MAX_ATR_SIZE];
  int target_present;
  size_t atr_len = sizeof atr;
  nfc_target nt;

  pnd->last_error = pcsc_get_status(pnd, &target_present, atr, &atr_len);
  if (pnd->last_error != NFC_SUCCESS)
    return pnd->last_error;

  if (!target_present) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "No target present");
    return NFC_ENOTSUCHDEV;
  }

  if (pnt) {
    if (pcsc_props_to_target(pnd, ICC_TYPE_UNKNOWN, atr, atr_len, NULL, 0, pnt->nm.nmt, &nt) != NFC_SUCCESS
        || pnt->nm.nmt != nt.nm.nmt || pnt->nm.nbr != nt.nm.nbr) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Target doesn't meet requirements");
      return NFC_ENOTSUCHDEV;
    }
  }
  return NFC_SUCCESS;
}

static int pcsc_device_set_property_bool(struct nfc_device *pnd, const nfc_property property, const bool bEnable)
{
  (void) pnd;
  switch (property) {
    case NP_INFINITE_SELECT:
      // ignore
      return NFC_SUCCESS;
    case NP_AUTO_ISO14443_4:
      if ((bEnable == true) || (is_pcsc_reader_vendor_feitian(pnd)))
        return NFC_SUCCESS;
      break;
    case NP_EASY_FRAMING:
      if ((bEnable == true) || (is_pcsc_reader_vendor_feitian(pnd)))
        return NFC_SUCCESS;
      break;
    case NP_FORCE_ISO14443_A:
    case NP_HANDLE_CRC:
    case NP_HANDLE_PARITY:
    case NP_FORCE_SPEED_106:
      if (bEnable == true)
        return NFC_SUCCESS;
      break;
    case NP_ACCEPT_INVALID_FRAMES:
    case NP_ACCEPT_MULTIPLE_FRAMES:
      if (bEnable == false)
        return NFC_SUCCESS;
      break;
    case NP_ACTIVATE_FIELD:
      if (bEnable == false) {
        struct pcsc_data *data = pnd->driver_data;
        pcsc_reconnect(pnd, data->dwShareMode, data->ioCard.dwProtocol, SCARD_LEAVE_CARD);
      }
      return NFC_SUCCESS;
    default:
      break;
  }
  return NFC_EDEVNOTSUPP;
}

static int pcsc_get_supported_modulation(struct nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type **const supported_mt)
{
  (void) pnd;
  if (mode == N_TARGET || NULL == supported_mt)
    return NFC_EINVARG;
  *supported_mt = pcsc_supported_mts;
  return NFC_SUCCESS;
}

static int pcsc_get_supported_baud_rate(struct nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br)
{
  (void) pnd;
  (void) nmt;
  if (mode == N_TARGET || NULL == supported_br)
    return NFC_EINVARG;
  *supported_br = pcsc_supported_brs;
  return NFC_SUCCESS;
}

static int
pcsc_get_information_about(nfc_device *pnd, char **pbuf)
{
  struct pcsc_data *data = pnd->driver_data;
  LPBYTE   name = NULL, version = NULL, type = NULL, serial = NULL;
#ifdef __APPLE__
  DWORD    name_len = 0, version_len = 0,
           type_len = 0, serial_len = 0;
#else
  DWORD    name_len = SCARD_AUTOALLOCATE, version_len = SCARD_AUTOALLOCATE,
           type_len = SCARD_AUTOALLOCATE, serial_len = SCARD_AUTOALLOCATE;
#endif
  int res = NFC_SUCCESS;
  SCARDCONTEXT *pscc;

  if (!(pscc = pcsc_get_scardcontext())) {
    pnd->last_error = NFC_ESOFT;
    return pnd->last_error;
  }

  SCardGetAttrib(data->hCard, SCARD_ATTR_VENDOR_NAME, (LPBYTE)&name, &name_len);
  SCardGetAttrib(data->hCard, SCARD_ATTR_VENDOR_IFD_TYPE, (LPBYTE)&type, &type_len);
  SCardGetAttrib(data->hCard, SCARD_ATTR_VENDOR_IFD_VERSION, (LPBYTE)&version, &version_len);
  SCardGetAttrib(data->hCard, SCARD_ATTR_VENDOR_IFD_SERIAL_NO, (LPBYTE)&serial, &serial_len);

  *pbuf = malloc(name_len + type_len + version_len + serial_len + 30);
  if (! *pbuf) {
    res = NFC_ESOFT;
    goto error;
  }
  sprintf((char *) *pbuf,
          "%s"     // model
          "%s%s"   // version
          " (%s)"  // vendor
          "%s%s\n" // serial
          ,
          name && name_len > 0 && name[0] != '\0'
          ? (char *)name : "unknown model",
          version && version_len > 0 && version[0] != '\0'
          ? " " : "", version_len > 0 ? (char *)version : "",
          type && type_len > 0 && type[0] != '\0'
          ? (char *)type : "unknown vendor",
          serial && serial_len > 0 && serial[0] != '\0'
          ? "\nserial: " : "", serial_len > 0 ? (char *)serial : "");

error:
#ifdef __APPLE__
  if (pscc != NULL) {
    SCardReleaseContext(*pscc);
  }
  if (name != NULL) {
    free(name);
    name = NULL;
  }
  if (type != NULL) {
    free(type);
    type = NULL;
  }
  if (version != NULL) {
    free(version);
    version = NULL;
  }
  if (serial != NULL) {
    free(serial);
    serial = NULL;
  }
#else
  SCardFreeMemory(*pscc, name);
  SCardFreeMemory(*pscc, type);
  SCardFreeMemory(*pscc, version);
  SCardFreeMemory(*pscc, serial);
#endif

  pnd->last_error = res;
  return pnd->last_error;
}

const struct nfc_driver pcsc_driver = {
  .name                             = PCSC_DRIVER_NAME,
  .scan                             = pcsc_scan,
  .open                             = pcsc_open,
  .close                            = pcsc_close,
  .strerror                         = pcsc_strerror,

  .initiator_init                   = pcsc_initiator_init,
  .initiator_init_secure_element    = NULL, // No secure-element support
  .initiator_select_passive_target  = pcsc_initiator_select_passive_target,
  .initiator_poll_target            = pcsc_initiator_poll_target,
  .initiator_select_dep_target      = NULL,
  .initiator_deselect_target        = NULL,
  .initiator_transceive_bytes       = pcsc_initiator_transceive_bytes,
  .initiator_transceive_bits        = NULL,
  .initiator_transceive_bytes_timed = NULL,
  .initiator_transceive_bits_timed  = NULL,
  .initiator_target_is_present      = pcsc_initiator_target_is_present,

  .target_init           = NULL,
  .target_send_bytes     = NULL,
  .target_receive_bytes  = NULL,
  .target_send_bits      = NULL,
  .target_receive_bits   = NULL,

  .device_set_property_bool     = pcsc_device_set_property_bool,
  .device_set_property_int      = NULL,
  .get_supported_modulation     = pcsc_get_supported_modulation,
  .get_supported_baud_rate      = pcsc_get_supported_baud_rate,
  .device_get_information_about = pcsc_get_information_about,

  .abort_command  = NULL,  // Abort is not supported in this driver
  .idle           = NULL,
  .powerdown      = NULL,
};
