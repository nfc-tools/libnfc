/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2011, Romuald Conty
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

// Needed by Sleep() under Windows
#  include <winbase.h>
#  define sleep(X) Sleep( X * 1000)

#endif /* _UNISTD_H_ */

