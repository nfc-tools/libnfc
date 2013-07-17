/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tarti?re
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2013      Laurent Latil
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
 * @file i2c.h
 * @brief I2C driver header
 */

#ifndef __NFC_BUS_I2C_H__
#  define __NFC_BUS_I2C_H__

#  include <sys/time.h>

#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>

#  include <linux/i2c-dev.h>
#  include <nfc/nfc-types.h>

typedef void *i2c_device;
#  define INVALID_I2C_BUS (void*)(~1)
#  define INVALID_I2C_ADDRESS (void*)(~2)

i2c_device i2c_open(const char *pcI2C_busName, uint32_t devAddr);

void       i2c_close(const i2c_device id);

ssize_t    i2c_read(i2c_device id, uint8_t *pbtRx, const size_t szRx);

int        i2c_write(i2c_device id, const uint8_t *pbtTx, const size_t szTx);

char     **i2c_list_ports(void);

#endif // __NFC_BUS_I2C_H__
