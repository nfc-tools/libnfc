/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _LIBNFC_BITUTILS_H_
#define _LIBNFC_BITUTILS_H_

#include "defines.h"

byte oddparity(const byte bt);
void oddparity_bytes(const byte* pbtData, const ui32 uiLen, byte* pbtPar);

byte mirror(byte bt);
ui32 mirror32(ui32 ui32Bits);
ui64 mirror64(ui64 ui64Bits);
void mirror_bytes(byte *pbts, ui32 uiLen);

ui32 swap_endian32(const void* pui32);
ui64 swap_endian64(const void* pui64);

void append_iso14443a_crc(byte* pbtData, ui32 uiLen);

void print_hex(const byte* pbtData, const ui32 uiLen);
void print_hex_bits(const byte* pbtData, const ui32 uiBits);
void print_hex_par(const byte* pbtData, const ui32 uiBits, const byte* pbtDataPar);

#endif // _LIBNFC_BITUTILS_H_

