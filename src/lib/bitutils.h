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
 * @file bitutils.h
 * @brief
 */

#ifndef _LIBNFC_BITUTILS_H_
#define _LIBNFC_BITUTILS_H_

#include <stdint.h>

#include <nfc/nfc-types.h>

byte_t oddparity(const byte_t bt);
void oddparity_byte_ts(const byte_t* pbtData, const size_t szLen, byte_t* pbtPar);

byte_t mirror(byte_t bt);
uint32_t mirror32(uint32_t ui32Bits);
uint64_t mirror64(uint64_t ui64Bits);
void mirror_byte_ts(byte_t *pbts, size_t szLen);

uint32_t swap_endian32(const void* pui32);
uint64_t swap_endian64(const void* pui64);

void append_iso14443a_crc(byte_t* pbtData, size_t szLen);

void print_hex(const byte_t* pbtData, const size_t szLen);
void print_hex_bits(const byte_t* pbtData, const size_t szBits);
void print_hex_par(const byte_t* pbtData, const size_t szBits, const byte_t* pbtDataPar);

#endif // _LIBNFC_BITUTILS_H_

