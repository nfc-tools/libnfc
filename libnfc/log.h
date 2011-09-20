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

#ifdef HAS_LOG4C

#define LOGGING 1

#include <log4c.h>

int	 log_init (void);
int	 log_fini (void);
void	 log_put (char *category, int priority, char *format, ...);

#define NFC_PRIORITY_FATAL  LOG4C_PRIORITY_FATAL
#define NFC_PRIORITY_ALERT  LOG4C_PRIORITY_ALERT
#define NFC_PRIORITY_CRIT   LOG4C_PRIORITY_CRIT
#define NFC_PRIORITY_ERROR  LOG4C_PRIORITY_ERROR
#define NFC_PRIORITY_WARN   LOG4C_PRIORITY_WARN
#define NFC_PRIORITY_NOTICE LOG4C_PRIORITY_NOTICE
#define NFC_PRIORITY_INFO   LOG4C_PRIORITY_INFO
#define NFC_PRIORITY_DEBUG  LOG4C_PRIORITY_DEBUG
#define NFC_PRIORITY_TRACE  LOG4C_PRIORITY_TRACE

#else /* HAS_LOG4C */

#define log_init() (0)
#define log_fini() (0)
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

#endif /* HAS_LOG4C */

/**
 * @macro LOG_HEX
 * @brief Log a byte-array in hexadecimal format
 */
#  ifdef LOGGING
#    define LOG_HEX(pcTag, pbtData, szBytes) do { \
    size_t	 __szPos; \
    char	 __acBuf[1024]; \
    size_t	 __szBuf = 0; \
    snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%s: ", pcTag); \
    __szBuf += strlen (pcTag) + 2; \
    for (__szPos=0; __szPos < (size_t)(szBytes); __szPos++) { \
      snprintf (__acBuf + __szBuf, sizeof(__acBuf) - __szBuf, "%02x  ",((uint8_t *)(pbtData))[__szPos]); \
      __szBuf += 4; \
    } \
    log_put (LOG_CATEGORY, NFC_PRIORITY_TRACE, __acBuf); \
  } while (0);
#  else
#    define LOG_HEX(pcTag, pbtData, szBytes) do { \
    (void) pcTag; \
    (void) pbtData; \
    (void) szBytes; \
  } while (0);
#  endif

#endif
