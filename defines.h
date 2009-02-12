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

#ifndef _LIBNFC_DEFINES_H_
#define _LIBNFC_DEFINES_H_

typedef unsigned char       byte;
typedef unsigned char       ui8;
typedef unsigned short      ui16;
typedef unsigned int        ui32;
typedef unsigned long long  ui64;
typedef unsigned long       ulong;
typedef char                i8;
typedef short               i16;
typedef int                 i32;

#define null 0

typedef void*               dev_id; // Device Id
#define INVALID_DEVICE_ID   null
#define MAX_FRAME_LEN       264

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// #define _LIBNFC_VERBOSE_

#endif // _LIBNFC_DEFINES_H_
