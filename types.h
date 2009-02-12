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

#ifndef _LIBNFC_TYPES_H_
#define _LIBNFC_TYPES_H_

#include "defines.h"

typedef enum {
  false = 0x00, 
  true  = 0x01
}bool;

typedef enum {
  MT_ISO14443A_106  = 0x00,
  MT_FELICA_212     = 0x01,
  MT_FELICA_424     = 0x02,
  MT_ISO14443B_106  = 0x03,
  MT_TOPAZ_106      = 0x04,
}ModulationType;

typedef enum {
  MC_AUTH_A         = 0x60,
  MC_AUTH_B         = 0x61,
  MC_READ           = 0x30,
  MC_WRITE          = 0xA0,
  MC_TRANSFER       = 0xB0,
  MC_DECREMENT      = 0xC0,
  MC_INCREMENT      = 0xC1,
  MC_STORE          = 0xC2,
}MifareCmd;

typedef struct {
  byte abtKey[6];
  byte abtUid[4];
}MifareParamAuth;

typedef struct {
  byte abtData[16];
}MifareParamData;

typedef struct {
  byte abtValue[4];
}MifareParamValue;

typedef union {
  MifareParamAuth mpa;
  MifareParamData mpd;
  MifareParamValue mpv;
}MifareParam;

#endif // _LIBNFC_TYPES_H_
