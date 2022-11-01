
/**
 * @file pn71xx.h
 * @brief Driver for PN71XX using libnfc-nci library
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "pn71xx.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"

#include "linux_nfc_api.h"

#define PN71XX_DRIVER_NAME "pn71xx"

#define LOG_CATEGORY "libnfc.driver.pn71xx"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

const nfc_modulation_type pn71xx_supported_modulation_as_target[] = {NMT_ISO14443A, NMT_FELICA, NMT_ISO14443B, NMT_ISO14443BI, NMT_ISO14443B2SR, NMT_ISO14443B2CT, NMT_JEWEL, NMT_DEP, 0};
const nfc_modulation_type pn71xx_supported_modulation_as_initiator[] = {NMT_ISO14443A, NMT_FELICA, NMT_ISO14443B, NMT_ISO14443BI, NMT_ISO14443B2SR, NMT_ISO14443B2CT, NMT_JEWEL, NMT_DEP, 0};

const nfc_baud_rate pn71xx_iso14443a_supported_baud_rates[] = { NBR_847, NBR_424, NBR_212, NBR_106, 0 };
const nfc_baud_rate pn71xx_felica_supported_baud_rates[] = { NBR_424, NBR_212, 0 };
const nfc_baud_rate pn71xx_dep_supported_baud_rates[] = { NBR_424, NBR_212, NBR_106, 0 };
const nfc_baud_rate pn71xx_jewel_supported_baud_rates[] = { NBR_847, NBR_424, NBR_212, NBR_106, 0 };
const nfc_baud_rate pn71xx_iso14443b_supported_baud_rates[] = { NBR_847, NBR_424, NBR_212, NBR_106, 0 };

static nfcTagCallback_t TagCB;
static nfc_tag_info_t *TagInfo = NULL;

static void onTagArrival(nfc_tag_info_t *pTagInfo);
static void onTagDeparture(void);

/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */
/**
 * @brief Initialize libnfc_nci library to verify presence of PN71xx device.
 *
 * @param context NFC context.
 * @param connstrings array of 'nfc_connstring' buffer  (allocated by caller). It is used to store the
 *      connection info strings of devices found.
 * @param connstrings_len length of the connstrings array.
 * @return number of devices found.
 */
static size_t
pn71xx_scan(const nfc_context *context, nfc_connstring connstrings[], const size_t connstrings_len)
{
  size_t device_found = 0;

  if ((context == NULL) || (connstrings_len == 0)) return 0;

  if (nfcManager_doInitialize() == 0) {
    nfc_connstring connstring = "pn71xx";
    memcpy(connstrings[device_found++], connstring, sizeof(nfc_connstring));
  }

  return device_found;
}

/**
 * @brief Close connection to PN71xx by stopping the discovery loop and deinitializing the libnfc_nci library.
 *
 * @param pnd pointer on the device to close.
 */
static void
pn71xx_close(nfc_device *pnd)
{
  nfcManager_disableDiscovery();
  nfcManager_deregisterTagCallback();
  nfcManager_doDeinitialize();
  nfc_device_free(pnd);
  pnd = NULL;
}

/**
 * @brief Open a connection to PN71xx, starting the discovery loop for tag detection.
 *
 * @param context NFC context.
 * @param connstring connection info to the device
 * @return pointer to the device, or NULL in case of error.
 */
static nfc_device *
pn71xx_open(const nfc_context *context, const nfc_connstring connstring)
{
  nfc_device *pnd;

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "open: %s", connstring);

  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    return NULL;
  }

  pnd->driver = &pn71xx_driver;
  strcpy(pnd->name, "pn71xx-device");
  strcpy(pnd->connstring, connstring);

  TagCB.onTagArrival = onTagArrival;
  TagCB.onTagDeparture = onTagDeparture;
  nfcManager_registerTagCallback(&TagCB);

  nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 1, 0, 0);

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "wait 1 seconds for polling");
  sleep(1);

  return pnd;
}

/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */
static bool IsTechnology(nfc_tag_info_t *TagInfo, nfc_modulation_type nmt)
{
  switch (nmt) {
    case NMT_ISO14443A:
      if (TagInfo->technology == TARGET_TYPE_ISO14443_4
          || TagInfo->technology == TARGET_TYPE_ISO14443_3A
          || TagInfo->technology == TARGET_TYPE_MIFARE_CLASSIC
          || TagInfo->technology == TARGET_TYPE_MIFARE_UL)
        return true;
      break;

    case NMT_ISO14443B:
    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
      if (TagInfo->technology == TARGET_TYPE_ISO14443_3B)
        return true;
      break;

    case NMT_FELICA:
      if (TagInfo->technology == TARGET_TYPE_FELICA)
        return true;
      break;

    case NMT_JEWEL:
      if (TagInfo->technology == TARGET_TYPE_ISO14443_3A
          && TagInfo->protocol == NFA_PROTOCOL_T1T)
        return true;
      break;

    default:
      return false;
  }
  return false;
}

static void BufferPrintBytes(char *buffer, unsigned int buflen, const uint8_t *data, unsigned int datalen)
{
  int cx = 0;
  for (unsigned int i = 0x00; i < datalen; i++)	{
    cx += snprintf(buffer + cx, buflen - cx, "%02X ", data[i]);
  }
}

static void PrintTagInfo(nfc_tag_info_t *TagInfo)
{
  switch (TagInfo->technology) {
    case TARGET_TYPE_UNKNOWN: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type Unknown'");
    }
    break;
    case TARGET_TYPE_ISO14443_3A: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type 3A'");
    }
    break;
    case TARGET_TYPE_ISO14443_3B: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type 3B'");
    }
    break;
    case TARGET_TYPE_ISO14443_4: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type 4A'");
    }
    break;
    case TARGET_TYPE_FELICA: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type F'");
    }
    break;
    case TARGET_TYPE_ISO15693: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type V'");
    }
    break;
    case TARGET_TYPE_NDEF: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type NDEF'");
    }
    break;
    case TARGET_TYPE_NDEF_FORMATABLE: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type Formatable'");
    }
    break;
    case TARGET_TYPE_MIFARE_CLASSIC: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Mifare Classic'");
    }
    break;
    case TARGET_TYPE_MIFARE_UL: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Mifare Ul'");
    }
    break;
    case TARGET_TYPE_KOVIO_BARCODE: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Kovio Barcode'");
    }
    break;
    case TARGET_TYPE_ISO14443_3A_3B: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A/B'");
    }
    break;
    default: {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type %d (Unknown or not supported)'\n", TagInfo->technology);
    }
    break;
  }
  /*32 is max UID len (Kovio tags)*/
  if ((0x00 != TagInfo->uid_length) && (32 >= TagInfo->uid_length)) {
    char buffer [100];
    int cx = 0;

    if (4 == TagInfo->uid_length || 7 == TagInfo->uid_length || 10 == TagInfo->uid_length) {
      cx += snprintf(buffer + cx, sizeof(buffer) - cx, "NFCID1 :    \t'");
    } else if (8 == TagInfo->uid_length) {
      cx += snprintf(buffer + cx, sizeof(buffer) - cx, "NFCID2 :    \t'");
    } else {
      cx += snprintf(buffer + cx, sizeof(buffer) - cx, "UID :    \t'");
    }

    BufferPrintBytes(buffer + cx, sizeof(buffer) - cx, (unsigned char *) TagInfo->uid, TagInfo->uid_length);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s'", buffer);
  }
}

/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */

static void onTagArrival(nfc_tag_info_t *pTagInfo)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "tag found");

  TagInfo = malloc(sizeof(nfc_tag_info_t));
  memcpy(TagInfo, pTagInfo, sizeof(nfc_tag_info_t));

  PrintTagInfo(TagInfo);
}

static void onTagDeparture(void)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "tag lost");

  free(TagInfo);
  TagInfo = NULL;
}

static int
pn71xx_initiator_init(struct nfc_device *pnd)
{
  if (pnd == NULL) return NFC_EIO;
  return NFC_SUCCESS;
}

static int
pn71xx_initiator_select_passive_target(struct nfc_device *pnd,
                                       const nfc_modulation nm,
                                       const uint8_t *pbtInitData, const size_t szInitData,
                                       nfc_target *pnt)
{
  if (pnd == NULL) return NFC_EIO;

  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "select_passive_target");

  if (TagInfo) {

    nfc_target nttmp;
    memset(&nttmp, 0x00, sizeof(nfc_target));
    nttmp.nm = nm;

    void *uidPtr = NULL;
    unsigned int maxLen = 0;

    switch (nm.nmt) {
      case NMT_ISO14443A:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 10;
          uidPtr = nttmp.nti.nai.abtUid;

          if (TagInfo->technology == TARGET_TYPE_MIFARE_CLASSIC) {
            nttmp.nti.nai.btSak = 0x08;
          } else {
            // make hardcoded desfire for freefare lib check
            nttmp.nti.nai.btSak = 0x20;
            nttmp.nti.nai.szAtsLen = 5;
            memcpy(nttmp.nti.nai.abtAts, "\x75\x77\x81\x02", 4);
          }
        }
        break;

      case NMT_ISO14443B:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 4;
          uidPtr = nttmp.nti.nbi.abtPupi;
        }
        break;

      case NMT_ISO14443BI:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 4;
          uidPtr = nttmp.nti.nii.abtDIV;
        }
        break;

      case NMT_ISO14443B2SR:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 8;
          uidPtr = nttmp.nti.nsi.abtUID;
        }
        break;

      case NMT_ISO14443B2CT:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 4;
          uidPtr = nttmp.nti.nci.abtUID;
        }
        break;

      case NMT_FELICA:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 8;
          uidPtr = nttmp.nti.nfi.abtId;
        }
        break;

      case NMT_JEWEL:
        if (IsTechnology(TagInfo, nm.nmt)) {
          maxLen = 4;
          uidPtr = nttmp.nti.nji.btId;
        }
        break;

      default:
        return 0;
    }

    if (uidPtr && TagInfo->uid_length) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "target found");
      int len = TagInfo->uid_length > maxLen ? maxLen : TagInfo->uid_length;
      memcpy(uidPtr, TagInfo->uid, len);
      if (nm.nmt == NMT_ISO14443A)
        nttmp.nti.nai.szUidLen = len;

      // Is a tag info struct available
      if (pnt) {
        memcpy(pnt, &nttmp, sizeof(nfc_target));
      }
      return 1;
    }
  }

  return 0;
}

static int
pn71xx_initiator_deselect_target(struct nfc_device *pnd)
{
  if (pnd == NULL) return NFC_EIO;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "deselect_passive_target");
  return NFC_SUCCESS;
}


static int
pn71xx_initiator_transceive_bytes(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx,
                                  const size_t szRx, int timeout)
{
  if (pnd == NULL) return NFC_EIO;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "transceive_bytes  timeout=%d", timeout);

  if (!TagInfo) return NFC_EINVARG;

  char buffer[500];
  BufferPrintBytes(buffer, sizeof(buffer), pbtTx, szTx);
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "===> %s", buffer);

  int received = nfcTag_transceive(TagInfo->handle, (uint8_t *) pbtTx, szTx, pbtRx, szRx, 500);
  if (received <= 0)
    return NFC_EIO;

  BufferPrintBytes(buffer, sizeof(buffer), pbtRx, received);
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "<=== %s", buffer);

  return received;
}

static int
pn71xx_initiator_poll_target(struct nfc_device *pnd,
                             const nfc_modulation *pnmModulations, const size_t szModulations,
                             const uint8_t uiPollNr, const uint8_t uiPeriod,
                             nfc_target *pnt)
{
  static int periodFactor = 150000;
  int period = uiPeriod * periodFactor;

  if (pnd == NULL) return 0;

  for (int j = 0; j < uiPollNr; j++) {
    for (unsigned int i = 0; i < szModulations; i++) {
      const nfc_modulation nm = pnmModulations[i];

      nfc_target nt;
      int res = pn71xx_initiator_select_passive_target(pnd, nm, 0, 0, &nt);
      if (res > 0 && pnt) {
        memcpy(pnt, &nt, sizeof(nfc_target));
        return res;
      }
    }
    usleep(period);
  }

  return 0;
}

static int
pn71xx_initiator_target_is_present(struct nfc_device *pnd, const nfc_target *pnt)
{
  if ((pnd == NULL) || (pnt == NULL)) return 1;
  return !TagInfo;
}


/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */
static int
pn71xx_get_supported_modulation(nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type **const supported_mt)
{
  if (pnd == NULL) return NFC_EIO;

  switch (mode) {
    case N_TARGET:
      *supported_mt = (nfc_modulation_type *)pn71xx_supported_modulation_as_target;
      break;
    case N_INITIATOR:
      *supported_mt = (nfc_modulation_type *)pn71xx_supported_modulation_as_initiator;
      break;
    default:
      return NFC_EINVARG;
  }
  return NFC_SUCCESS;
}

static int
pn71xx_get_supported_baud_rate(nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br)
{
  if (pnd == NULL) return NFC_EIO;
  if (mode) {}

  switch (nmt) {
    case NMT_FELICA:
      *supported_br = (nfc_baud_rate *)pn71xx_felica_supported_baud_rates;
      break;
    case NMT_ISO14443A:
      *supported_br = (nfc_baud_rate *)pn71xx_iso14443a_supported_baud_rates;
      break;
    case NMT_ISO14443B:
    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
      *supported_br = (nfc_baud_rate *)pn71xx_iso14443b_supported_baud_rates;
      break;
    case NMT_JEWEL:
      *supported_br = (nfc_baud_rate *)pn71xx_jewel_supported_baud_rates;
      break;
    case NMT_DEP:
      *supported_br = (nfc_baud_rate *)pn71xx_dep_supported_baud_rates;
      break;
    default:
      return NFC_EINVARG;
  }
  return NFC_SUCCESS;
}

/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */

static int
pn71xx_set_property_bool(struct nfc_device *pnd, const nfc_property property, const bool bEnable)
{
  if (pnd == NULL) return NFC_EIO;
  return NFC_SUCCESS;
}

static int
pn71xx_set_property_int(struct nfc_device *pnd, const nfc_property property, const int value)
{
  if (pnd == NULL) return NFC_EIO;
  return NFC_SUCCESS;
}

static int
pn71xx_get_information_about(nfc_device *pnd, char **pbuf)
{
  static const char *info = "PN71XX nfc driver using libnfc-nci userspace library";
  size_t buflen = strlen(info) + 1;

  if (pnd == NULL) return NFC_EIO;

  *pbuf = malloc(buflen);
  memcpy(*pbuf, info, buflen);

  return buflen;
}
/**
 * @brief Abort any pending operation
 *
 * @param pnd pointer on the NFC device.
 * @return NFC_SUCCESS
 */
static int
pn71xx_abort_command(nfc_device *pnd)
{
  if (pnd == NULL) return NFC_EIO;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "abort_command");
  return NFC_SUCCESS;
}

static int
pn71xx_idle(struct nfc_device *pnd)
{
  if (pnd == NULL) return NFC_EIO;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "idle");
  return NFC_SUCCESS;
}

static int
pn71xx_PowerDown(struct nfc_device *pnd)
{
  if (pnd == NULL) return NFC_EIO;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "PowerDown");
  return NFC_SUCCESS;
}

/** ------------------------------------------------------------------------ */
/** ------------------------------------------------------------------------ */
const struct nfc_driver pn71xx_driver = {
  .name                             = PN71XX_DRIVER_NAME,
  .scan_type                        = NOT_INTRUSIVE,
  .scan                             = pn71xx_scan,
  .open                             = pn71xx_open,
  .close                            = pn71xx_close,
  .strerror                         = NULL,

  .initiator_init                   = pn71xx_initiator_init,
  .initiator_init_secure_element    = NULL,
  .initiator_select_passive_target  = pn71xx_initiator_select_passive_target,
  .initiator_poll_target            = pn71xx_initiator_poll_target,
  .initiator_select_dep_target      = NULL,
  .initiator_deselect_target        = pn71xx_initiator_deselect_target,
  .initiator_transceive_bytes       = pn71xx_initiator_transceive_bytes,
  .initiator_transceive_bits        = NULL,
  .initiator_transceive_bytes_timed = NULL,
  .initiator_transceive_bits_timed  = NULL,
  .initiator_target_is_present      = pn71xx_initiator_target_is_present,

  .target_init                      = NULL,
  .target_send_bytes                = NULL,
  .target_receive_bytes             = NULL,
  .target_send_bits                 = NULL,
  .target_receive_bits              = NULL,

  .device_set_property_bool         = pn71xx_set_property_bool,
  .device_set_property_int          = pn71xx_set_property_int,
  .get_supported_modulation         = pn71xx_get_supported_modulation,
  .get_supported_baud_rate          = pn71xx_get_supported_baud_rate,
  .device_get_information_about     = pn71xx_get_information_about,

  .abort_command  = pn71xx_abort_command,
  .idle           = pn71xx_idle,
  .powerdown      = pn71xx_PowerDown,
};

