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

// Handle platform specific includes
#include "contrib/windows.h"

int setenv(const char *name, const char *value, int overwrite)
{
  int exists = GetEnvironmentVariableA(name, NULL, 0);
  if ((exists && overwrite) || (!exists)) {
    if (!SetEnvironmentVariableA(name, value)) {
      // Set errno here correctly
      return -1;
    }
    return 0;
  }
  // Exists and overwrite is 0.
  return -1;
}

void unsetenv(const char *name)
{
  SetEnvironmentVariableA(name, NULL);
}
