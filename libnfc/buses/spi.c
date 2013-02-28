/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2013 Evgeny Boger
 * Copyright (C) 2009, 2010 Roel Verdult
 * Copyright (C) 2009, 2010 Romuald Conty
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
 * @file spi.c
 * @brief SPI driver wrapper
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "spi.h"

#include <nfc/nfc.h>
#include "nfc-internal.h"

// Test if we are dealing with unix operating systems
#ifndef _WIN32
// The POSIX SPI port implementation
#  include "spi_posix.c"
#else
// The windows SPI port implementation
# error "Not implemented"
#endif /* _WIN32 */
