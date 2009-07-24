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

#ifndef _LIBNFC_MIFARE_UL_TAG_H_
#define _LIBNFC_MIFARE_UL_TAG_H_

#include "defines.h"

typedef struct {
  byte_t sn0[3];
  byte_t btBCC0;
  byte_t sn1[4];
  byte_t btBCC1;
  byte_t internal;
  byte_t lock[2];
  byte_t otp[4];
} mifareul_block_manufacturer;

typedef struct {
  byte_t abtData[16];
} mifareul_block_data;

typedef union {
  mifareul_block_manufacturer mbm;
  mifareul_block_data mbd;
} mifareul_block;

typedef struct {
  mifareul_block amb[4];
} mifareul_tag;

#endif // _LIBNFC_MIFARE_UL_TAG_H_
