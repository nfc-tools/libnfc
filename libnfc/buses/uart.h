/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, 2010, Roel Verdult, Romuald Conty
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
 * @file uart.h
 * @brief UART driver header
 */

#ifndef __NFC_BUS_UART_H__
#  define __NFC_BUS_UART_H__

#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>


#  include <nfc/nfc-types.h>

// Handle platform specific includes
#  ifndef _WIN32
#    include <termios.h>
#    include <sys/ioctl.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <limits.h>
#    include <sys/time.h>
#    include <unistd.h>
#    define delay_ms( X ) usleep( X * 1000 )
#  else
#    include "contrib/windows.h"
#    define delay_ms( X ) Sleep( X )
#  endif

// Path to the serial port is OS-dependant.
// Try to guess what we should use.
#  if defined (_WIN32)
#    define DEFAULT_SERIAL_PORTS { "COM1", "COM2", "COM3", "COM4", NULL }
#  elif defined(__APPLE__)
  // XXX: find UART connection string for PN53X device on Mac OS X when multiples devices are used
#    define DEFAULT_SERIAL_PORTS { "/dev/tty.SLAB_USBtoUART", NULL }
#  elif defined (__FreeBSD__) || defined (__OpenBSD__)
  // XXX: Not tested
#    define DEFAULT_SERIAL_PORTS { "/dev/cuau0", "/dev/cuau1", "/dev/cuau2", "/dev/cuau3", NULL }
#  elif defined (__linux__)
#    define DEFAULT_SERIAL_PORTS { "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3", "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3", NULL }
#  else
#    error "Can't determine serial string for your system"
#  endif

// Define shortcut to types to make code more readable
typedef void *serial_port;
#  define INVALID_SERIAL_PORT (void*)(~1)
#  define CLAIMED_SERIAL_PORT (void*)(~2)

serial_port uart_open (const char *pcPortName);
void    uart_close (const serial_port sp);

void    uart_set_speed (serial_port sp, const uint32_t uiPortSpeed);
uint32_t uart_get_speed (const serial_port sp);

int     uart_receive (serial_port sp, byte_t * pbtRx, size_t * pszRx);
int     uart_send (serial_port sp, const byte_t * pbtTx, const size_t szTx);

#endif // __NFC_BUS_UART_H__
