/*-
 * Copyright (C) 2011 Romain Tartière
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

#include "log.h"

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

void 
log_init(const nfc_context *context)
{
  char str[32];
  sprintf(str, "%"PRIu32, context->log_level);
  setenv("LIBNFC_LOG_LEVEL", str, 1);
}

void
log_exit(void)
{
}

void
log_put(const uint8_t group, const char *category, const uint8_t priority, const char *format, ...)
{
  char *env_log_level = getenv("LIBNFC_LOG_LEVEL");
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
    if ( ((log_level & 0x00000003) >= priority) ||  // Global log level
         (((log_level >> (group*2)) & 0x00000003) >= priority) ) { // Group log level
      va_list va;
      va_start(va, format);
      fprintf(stderr, "%s\t%s\t", log_priority_to_str(priority), category);
      vfprintf(stderr, format, va);
      fprintf(stderr, "\n");
      va_end(va);
    }
  }
}
