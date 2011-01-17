/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Romuald Conty
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
 * @file windows.h
 * @brief Provide some windows related hacks due to lack of POSIX compat
 */

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#  include <windows.h>
#  if defined (__MINGW32__)
#    define snprintf(S, n, F, ...) sprintf(S, F, __VA_ARGS__)
#    define MAX(a,b) max(a,b)
#    define MIN(a,b) min(a,b)
#  else
#    define snprintf sprintf_s
#    define strdup _strdup
#  endif

#endif
