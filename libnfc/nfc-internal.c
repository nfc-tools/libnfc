/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2011, 2012 Romuald Conty
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
* @file nfc-internal.c
* @brief Provide some useful internal functions
*/

#include <nfc/nfc.h>
#include "nfc-internal.h"
#include "conf.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define LOG_GROUP    NFC_LOG_GROUP_GENERAL
#define LOG_CATEGORY "libnfc.general"

void 
string_as_boolean(const char* s, bool *value)
{
  if (s) {
    if (!(*value)) {
      if ( (strcmp(s, "yes") == 0) ||
           (strcmp(s, "true") == 0) ||
           (strcmp(s, "1") == 0) ) {
           *value = true;
           return;
      }
    } else {
      if ( (strcmp(s, "no") == 0) ||
           (strcmp(s, "false") == 0) ||
           (strcmp(s, "0") == 0) ) {
           *value = false;
           return;
      }
    }
  }
}

nfc_context *
nfc_context_new(void)
{
  nfc_context *res = malloc(sizeof(*res));

  if (!res) {
    err(EXIT_FAILURE, "nfc_context_new: malloc");
  }

  // Set default context values
  res->allow_autoscan = true;
  res->allow_intrusive_scan = false;
#ifdef DEBUG
  res->log_level = 3;
#else
  res->log_level = 1;
#endif

  // Load options from configuration file (ie. /etc/nfc/libnfc.conf)
  conf_load(res);

  // Environment variables
  // Load "intrusive scan" option
  char *envvar = getenv("LIBNFC_INTRUSIVE_SCAN");
  string_as_boolean(envvar, &(res->allow_intrusive_scan));

  // log level
  envvar = getenv("LIBNFC_LOG_LEVEL");
  if (envvar) {
    res->log_level = atoi(envvar);
  }

  // Debug context state
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_NONE,  "log_level is set to %"PRIu32, res->log_level);
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "allow_autoscan is set to %s", (res->allow_autoscan)?"true":"false");
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "allow_intrusive_scan is set to %s", (res->allow_intrusive_scan)?"true":"false");
  return res;
}

void
nfc_context_free(nfc_context *context)
{
  free(context);
}

void
prepare_initiator_data(const nfc_modulation nm, uint8_t **ppbtInitiatorData, size_t *pszInitiatorData)
{
  switch (nm.nmt) {
    case NMT_ISO14443B: {
      // Application Family Identifier (AFI) must equals 0x00 in order to wakeup all ISO14443-B PICCs (see ISO/IEC 14443-3)
      *ppbtInitiatorData = (uint8_t *) "\x00";
      *pszInitiatorData = 1;
    }
    break;
    case NMT_ISO14443BI: {
      // APGEN
      *ppbtInitiatorData = (uint8_t *) "\x01\x0b\x3f\x80";
      *pszInitiatorData = 4;
    }
    break;
    case NMT_ISO14443B2SR: {
      // Get_UID
      *ppbtInitiatorData = (uint8_t *) "\x0b";
      *pszInitiatorData = 1;
    }
    break;
    case NMT_ISO14443B2CT: {
      // SELECT-ALL
      *ppbtInitiatorData = (uint8_t *) "\x9F\xFF\xFF";
      *pszInitiatorData = 3;
    }
    break;
    case NMT_FELICA: {
      // polling payload must be present (see ISO/IEC 18092 11.2.2.5)
      *ppbtInitiatorData = (uint8_t *) "\x00\xff\xff\x01\x00";
      *pszInitiatorData = 5;
    }
    break;
    case NMT_ISO14443A:
    case NMT_JEWEL:
    case NMT_DEP:
      *ppbtInitiatorData = NULL;
      *pszInitiatorData = 0;
      break;
  }
}
