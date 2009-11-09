/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * 
 * @file nfc-defines.h
 * @brief
 */

#ifndef __NFC_DEFINES_H__
#define __NFC_DEFINES_H__

/* DEBUG flag can also be enabled using ./configure --enable-debug */
/* #define DEBUG */

#define INVALID_DEVICE_INFO 0
#define MAX_FRAME_LEN       264
#define MAX_DEVICES         16

#endif // _LIBNFC_DEFINES_H_
