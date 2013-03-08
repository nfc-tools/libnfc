/*-
 * Copyright (C) 2012 Romuald Conty
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

#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

/*
int
log_priority_to_int(const char* priority)
{
  if (strcmp("none", priority) == 0) {
    return -1;
  } else if (strcmp("fatal", priority) == 0) {
    return NFC_LOG_PRIORITY_FATAL;
  } else if (strcmp("alert", priority) == 0) {
    return NFC_LOG_PRIORITY_ALERT;
  } else if (strcmp("critical", priority) == 0) {
    return NFC_LOG_PRIORITY_CRIT;
  } else if (strcmp("error", priority) == 0) {
    return NFC_LOG_PRIORITY_ERROR;
  } else if (strcmp("warning", priority) == 0) {
    return NFC_LOG_PRIORITY_WARN;
  } else if (strcmp("notice", priority) == 0) {
    return NFC_LOG_PRIORITY_NOTICE;
  } else if (strcmp("info", priority) == 0) {
    return NFC_LOG_PRIORITY_INFO;
  } else if (strcmp("debug", priority) == 0) {
    return NFC_LOG_PRIORITY_DEBUG;
  } else if (strcmp("trace", priority) == 0) {
    return NFC_LOG_PRIORITY_TRACE;
  }

  // if priority is string is not recognized, we set maximal verbosity
  return NFC_LOG_PRIORITY_TRACE;
}
*/

const char *
log_priority_to_str(const int priority)
{
  switch (priority) {
    case NFC_LOG_PRIORITY_ERROR:
      return  "error";
    case NFC_LOG_PRIORITY_INFO:
      return  "info";
    case NFC_LOG_PRIORITY_DEBUG:
      return  "debug";
    default:
      break;
  }
  return "unknown";
}


#ifdef LOG

#include "log_internal.h"

void
log_init(const nfc_context *context)
{
#ifdef ENVVARS
  char str[32];
  sprintf(str, "%"PRIu32, context->log_level);
  setenv("LIBNFC_LOG_LEVEL", str, 1);
#else
  (void)context;
#endif
}

void
log_exit(void)
{
}

void
log_put(const uint8_t group, const char *category, const uint8_t priority, const char *format, ...)
{
  char *env_log_level = NULL;
#ifdef ENVVARS
  env_log_level = getenv("LIBNFC_LOG_LEVEL");
#endif
  uint32_t log_level;
  if (NULL == env_log_level) {
    // LIBNFC_LOG_LEVEL is not set
#ifdef DEBUG
    log_level = 3;
#else
    log_level = 1;
#endif
  } else {
    log_level = atoi(env_log_level);
  }

  //  printf("log_level = %"PRIu32" group = %"PRIu8" priority = %"PRIu8"\n", log_level, group, priority);
  if (log_level) { // If log is not disabled by log_level=none
    if (((log_level & 0x00000003) >= priority) ||   // Global log level
        (((log_level >> (group * 2)) & 0x00000003) >= priority)) { // Group log level

      va_list va;
      va_start(va, format);
      log_put_internal("%s\t%s\t", log_priority_to_str(priority), category);
      log_vput_internal(format, va);
      log_put_internal("\n");
      va_end(va);
    }
  }
}

#endif // LOG
