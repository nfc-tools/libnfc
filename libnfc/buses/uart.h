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
 *
 */

/**
 * @file uart.h
 * @brief UART driver header
 */

#ifndef __NFC_BUS_UART_H__
#  define __NFC_BUS_UART_H__

#  include <sys/time.h>

#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>


#  include <nfc/nfc-types.h>

// Define shortcut to types to make code more readable
typedef void *serial_port;
#  define INVALID_SERIAL_PORT (void*)(~1)
#  define CLAIMED_SERIAL_PORT (void*)(~2)

serial_port uart_open(const char *pcPortName);
void    uart_close(const serial_port sp);
void    uart_flush_input(const serial_port sp, bool wait);

void    uart_set_speed(serial_port sp, const uint32_t uiPortSpeed);
uint32_t uart_get_speed(const serial_port sp);

int     uart_receive(serial_port sp, uint8_t *pbtRx, const size_t szRx, void *abort_p, int timeout);
int     uart_send(serial_port sp, const uint8_t *pbtTx, const size_t szTx, int timeout);

char  **uart_list_ports(void);

#endif // __NFC_BUS_UART_H__
