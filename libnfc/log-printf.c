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

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#include "log.h"

static uint8_t __log_init_counter = 0;

int
log_init(void)
{
  int res = 0;

  if (__log_init_counter == 0) {
    res = 0;
  }
  if (!res) {
    __log_init_counter++;
  }
  return res;
}

int
log_fini(void)
{
  int res = 0;
  if (__log_init_counter >= 1) {
    if (__log_init_counter == 1) {
      res = 0;
    }
    __log_init_counter--;
  } else {
    res = -1;
  }
  return res;
}

void
log_put(const char *category, const char *priority, const char *format, ...)
{
  va_list va;
  va_start(va, format);
  fprintf(stderr, "%s\t%s\t", priority, category);
  vfprintf(stderr, format, va);
  fprintf(stderr, "\n");
  va_end(va);
}
