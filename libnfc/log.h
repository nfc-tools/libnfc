/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "nfc-internal.h"

#define NFC_LOG_PRIORITY_NONE   0
#define NFC_LOG_PRIORITY_ERROR  1
#define NFC_LOG_PRIORITY_INFO   2
#define NFC_LOG_PRIORITY_DEBUG  3

#define NFC_LOG_GROUP_GENERAL   1
#define NFC_LOG_GROUP_CONFIG    2
#define NFC_LOG_GROUP_CHIP      3
#define NFC_LOG_GROUP_DRIVER    4
#define NFC_LOG_GROUP_COM       5
#define NFC_LOG_GROUP_LIBUSB    6

/*
  To enable log only for one (or more) group, you can use this formula:
    log_level = NFC_LOG_PRIORITY(main) + NFC_LOG_PRIORITY(group) * 2 ^ (NFC_LOG_GROUP(group) * 2)

  Examples:
   * Main log level is NONE and only communication group log is set to DEBUG verbosity (for rx/tx trace):
       LIBNFC_LOG_LEVEL=3072  // 0+3072
   * Main log level is ERROR and driver layer log is set to DEBUG level:
       LIBNFC_LOG_LEVEL=769   // 1+768
   * Main log level is ERROR, driver layer is set to INFO and communication is set to DEBUG:
       LIBNFC_LOG_LEVEL=3585  // 1+512+3072
*/

//int log_priority_to_int(const char* priority);
const char *log_priority_to_str(const int priority);

#if defined LOG

#  ifndef __has_attribute
#    define __has_attribute(x) 0
#  endif

#  if __has_attribute(format) || defined(__GNUC__)
#    define __has_attribute_format 1
#  endif

void log_init(const nfc_context *context);
void log_exit(void);
void log_put(const uint8_t group, const char *category, const uint8_t priority, const char *format, ...)
#  if __has_attribute_format
__attribute__((format(printf, 4, 5)))
#  endif
;
#else
// No logging
#define log_init(nfc_context) ((void) 0)
#define log_exit() ((void) 0)
#define log_put(group, category, priority, format, ...) do {} while (0)

#endif // LOG

/**
 * @macro LOG_HEX
 * @brief Log a byte-array in hexadecimal format
 * Max values:  pcTag of 121 bytes + ": " + 300 bytes of data+ "\0" => acBuf of 1024 bytes
 */
#  ifdef LOG
#    define LOG_HEX(group, pcTag, pbtData, szBytes) do { \
    size_t	 __szPos; \
    char	 __acBuf[1024]; \
    size_t	 __szBuf = 0; \
    if ((int)szBytes < 0) { \
      fprintf (stderr, "%s:%d: Attempt to print %d bytes!\n", __FILE__, __LINE__, (int)szBytes); \
      log_put (group, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s:%d: Attempt to print %d bytes!\n", __FILE__, __LINE__, (int)szBytes); \
      abort(); \
      break; \
    } \
    snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%s: ", pcTag); \
    __szBuf += strlen (pcTag) + 2; \
    for (__szPos=0; (__szPos < (size_t)(szBytes)) && (__szBuf < sizeof(__acBuf)); __szPos++) { \
      snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%02x ",((uint8_t *)(pbtData))[__szPos]); \
      __szBuf += 3; \
    } \
    log_put (group, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", __acBuf); \
  } while (0);
#  else
#    define LOG_HEX(group, pcTag, pbtData, szBytes) do { \
    (void) group; \
    (void) pcTag; \
    (void) pbtData; \
    (void) szBytes; \
  } while (0);
#  endif

#endif // __LOG_H__
