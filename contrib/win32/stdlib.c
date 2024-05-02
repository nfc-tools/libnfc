/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2013      Alex Lian
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
 *
 */

/**
 * @file stdlib.c
 * @brief Windows System compatibility
 */

#include <stddef.h>
#include <stdlib.h>

#include "contrib/windows.h"

// This implementation of setenv() and unsetenv() using _putenv_s() as the
// underlying function.
// NOTE: unlike the POSIX functions, they return errno instead of -1 when they
// fail, and they allow name to contain '=', which is not allowed by the POSIX
// standard.
int setenv(const char *name, const char *value, int overwrite)
{
  if (!overwrite) {
    size_t sz;
    // Test for existence.
    getenv_s(&sz, NULL, 0, name);
    if (sz != 0)
      return 0;
  }
  return _putenv_s(name, value);
}

int unsetenv(const char *name)
{
  return _putenv_s(name, "");
}

#ifdef NEED_LIBNFC_SNPRINTF

#include <stdarg.h>
#include <stdio.h>

int libnfc_vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
  int retval;

  /* _vsnprintf() does not work with zero length buffer
   * so count number of character by _vscprintf() call */
  if (n == 0)
    return _vscprintf(format, arg);

  retval = _vsnprintf(s, n, format, arg);

  /* _vsnprintf() does not fill trailing null byte if there is not place for it
   */
  if (retval < 0 || (size_t)retval == n)
    s[n - 1] = '\0';

  /* _vsnprintf() returns negative number if buffer is too small
   * so count number of character by _vscprintf() call */
  if (retval < 0)
    retval = _vscprintf(format, arg);

  return retval;
}

int libnfc_snprintf(char *buffer, size_t n, const char *format, ...)
{
  int retval;
  va_list argptr;

  va_start(argptr, format);
  retval = libnfc_vsnprintf(buffer, n, format, argptr);
  va_end(argptr);
  return retval;
}

#endif
