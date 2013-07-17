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
 * Copyright (C) 2013      Evgeny Boger
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
 * @file spi.h
 * @brief SPI driver header
 */

#ifndef __NFC_BUS_SPI_H__
#  define __NFC_BUS_SPI_H__

#  include <sys/time.h>

#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>

#  include <linux/spi/spidev.h>

#  include <nfc/nfc-types.h>

// Define shortcut to types to make code more readable
typedef void *spi_port;
#  define INVALID_SPI_PORT (void*)(~1)
#  define CLAIMED_SPI_PORT (void*)(~2)

spi_port spi_open(const char *pcPortName);
void    spi_close(const spi_port sp);

void    spi_set_speed(spi_port sp, const uint32_t uiPortSpeed);
void    spi_set_mode(spi_port sp, const uint32_t uiPortMode);
uint32_t spi_get_speed(const spi_port sp);

int     spi_receive(spi_port sp, uint8_t *pbtRx, const size_t szRx, bool lsb_first);
int     spi_send(spi_port sp, const uint8_t *pbtTx, const size_t szTx, bool lsb_first);
int     spi_send_receive(spi_port sp, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, bool lsb_first);

char  **spi_list_ports(void);

#endif // __NFC_BUS_SPI_H__
