/*-
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
 */

 /**
 * @file iso14443-subr.c
 * @brief
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>

#include <nfc/nfc.h>

void
iso14443a_crc (byte_t * pbtData, size_t szLen, byte_t * pbtCrc)
{
  byte_t  bt;
  uint32_t wCrc = 0x6363;

  do {
    bt = *pbtData++;
    bt = (bt ^ (byte_t) (wCrc & 0x00FF));
    bt = (bt ^ (bt << 4));
    wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
  } while (--szLen);

  *pbtCrc++ = (byte_t) (wCrc & 0xFF);
  *pbtCrc = (byte_t) ((wCrc >> 8) & 0xFF);
}

void
iso14443a_crc_append (byte_t * pbtData, size_t szLen)
{
  iso14443a_crc (pbtData, szLen, pbtData + szLen);
}

byte_t *
iso14443a_locate_historical_bytes(byte_t * pbtAts, size_t szAts, size_t * pszTk)
{
  if (szAts) {
    size_t offset = 1;
    if (pbtAts[0] & 0x10) { // TA
      offset++;
    }
    if (pbtAts[0] & 0x20) { // TB
      offset++;
    }
    if (pbtAts[0] & 0x40) { // TC
      offset++;
    }
    if (szAts > offset) {
      *pszTk = (szAts-offset);
      return (pbtAts+offset);
    }
  }
  *pszTk = 0;
  return NULL;
}
