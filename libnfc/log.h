/*-
 * Copyright (C) 2011, Romain Tartière, Romuald Conty
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

#if defined DEBUG

#  ifndef __has_attribute
#    define __has_attribute(x) 0
#  endif

#  if __has_attribute(format) || defined(__GNUC__)
#    define __has_attribute_format 1
#  endif

// User want debug features
#define LOGGING 1
int	 log_init(void);
int	 log_fini(void);
void log_put(const char *category, const char *priority, const char *format, ...)
#  if __has_attribute_format
__attribute__((format(printf, 3, 4)))
#  endif
;


#define NFC_PRIORITY_FATAL  "fatal"
#define NFC_PRIORITY_ALERT  "alert"
#define NFC_PRIORITY_CRIT   "critical"
#define NFC_PRIORITY_ERROR  "error"
#define NFC_PRIORITY_WARN   "warning"
#define NFC_PRIORITY_NOTICE "notice"
#define NFC_PRIORITY_INFO   "info"
#define NFC_PRIORITY_DEBUG  "debug"
#define NFC_PRIORITY_TRACE  "trace"
#else
// No logging
#define log_init() ((void) 0)
#define log_fini() ((void) 0)
#define log_msg(category, priority, message) do {} while (0)
#define log_set_appender(category, appender) do {} while (0)
#define log_put(category, priority, format, ...) do {} while (0)

#define NFC_PRIORITY_FATAL  8
#define NFC_PRIORITY_ALERT  7
#define NFC_PRIORITY_CRIT   6
#define NFC_PRIORITY_ERROR  5
#define NFC_PRIORITY_WARN   4
#define NFC_PRIORITY_NOTICE 3
#define NFC_PRIORITY_INFO   2
#define NFC_PRIORITY_DEBUG  1
#define NFC_PRIORITY_TRACE  0
#endif /* HAS_LOG4C, DEBUG */

/**
 * @macro LOG_HEX
 * @brief Log a byte-array in hexadecimal format
 */
#  ifdef LOGGING
#    define LOG_HEX(pcTag, pbtData, szBytes) do { \
    size_t	 __szPos; \
    char	 __acBuf[1024]; \
    size_t	 __szBuf = 0; \
    if ((int)szBytes < 0) { \
      fprintf (stderr, "%s:%d: Attempt to print %d bytes!\n", __FILE__, __LINE__, (int)szBytes); \
      log_put (LOG_CATEGORY, NFC_PRIORITY_FATAL, "%s:%d: Attempt to print %d bytes!\n", __FILE__, __LINE__, (int)szBytes); \
      abort(); \
      break; \
    } \
    snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%s: ", pcTag); \
    __szBuf += strlen (pcTag) + 2; \
    for (__szPos=0; __szPos < (size_t)(szBytes); __szPos++) { \
      snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%02x  ",((uint8_t *)(pbtData))[__szPos]); \
      __szBuf += 4; \
    } \
    log_put (LOG_CATEGORY, NFC_PRIORITY_TRACE, "%s", __acBuf); \
  } while (0);
#  else
#    define LOG_HEX(pcTag, pbtData, szBytes) do { \
    (void) pcTag; \
    (void) pbtData; \
    (void) szBytes; \
  } while (0);
#  endif

#endif
