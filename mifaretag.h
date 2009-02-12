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

#ifndef _MIFARE_TAG_H_
#define _MIFARE_TAG_H_

#include "defines.h"

typedef struct tagBlockManufacturer {
  byte abtUID[4];
  byte btBCC;
  byte btUnknown;
  byte abtATQA[2];
  byte abtUnknown[8];
} BlockManufacturer;

typedef struct tagBlockData {
  byte abtContent[16];
} BlockData;

typedef struct tagBlockTrailer {
  byte abtKeyA[6];
  byte abtAccessBits[4];
  byte abtKeyB[6];
} BlockTrailer;

typedef union tagBlock {
  BlockManufacturer bm;
  BlockData bd;
  BlockTrailer bt;
} Block;

typedef struct tagMifareTag {
  Block blContent[256];
} MifareTag;

#endif // _MIFARE_TAG_H_
