/*-
 * Copyright (C) 2011 Romain Tartière
 * Copyright (C) 2011, 2012 Romuald Conty
 * Copyright (C) 2013 Alex Lian
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

void
log_vput_internal(const char *format, va_list args)
{
  vfprintf(stderr, format, args);
}

void
log_put_internal(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end(va);
}
