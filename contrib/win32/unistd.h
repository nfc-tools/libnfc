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
 * @file unistd.h
 * @brief This file intended to serve as a drop-in replacement for unistd.h on Windows
 */

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include "contrib/windows.h"

// Needed by Sleep() under Windows
#  include <winbase.h>
#  define sleep(X) Sleep( X * 1000)

// With MinGW, getopt(3) is provided as separate header
#if defined(WIN32) && defined(__GNUC__) /* mingw compiler */
#include <getopt.h>
#endif


#endif /* _UNISTD_H_ */

