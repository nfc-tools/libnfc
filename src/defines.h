/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#ifndef _LIBNFC_DEFINES_H_
#define _LIBNFC_DEFINES_H_

// #define DEBUG   /* DEBUG flag can also be enabled using ./configure --enable-debug */

typedef void*               dev_spec; // Device connection specification
#define INVALID_DEVICE_INFO 0
#define MAX_FRAME_LEN       264
#define DEVICE_NAME_LENGTH  256
#define MAX_DEVICES         16

// Useful macros
//#define MIN(a,b) (((a) < (b)) ? (a) : (b))
//#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define INNER_XOR8(n) {n ^= (n >> 4); n ^= (n >> 2); n ^= (n >> 1); n &= 0x01; }
#define INNER_XOR32(n) {n ^= (n >> 16); n ^= (n >> 8); INNER_XOR8(n); }
#define INNER_XOR64(n) {n ^= (n >> 32); INNER_XOR32(n); }

#endif // _LIBNFC_DEFINES_H_
