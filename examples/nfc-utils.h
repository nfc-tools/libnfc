/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Romuald Conty
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
 * @file nfc-utils.h
 * @brief Provide some examples shared functions like print, parity calculation, options parsing.
 */

#ifndef _EXAMPLES_NFC_UTILS_H_
#  define _EXAMPLES_NFC_UTILS_H_

#  include <stdlib.h>
#  include <string.h>

byte_t  oddparity (const byte_t bt);
void    oddparity_byte_ts (const byte_t * pbtData, const size_t szLen, byte_t * pbtPar);

void    print_hex (const byte_t * pbtData, const size_t szLen);
void    print_hex_bits (const byte_t * pbtData, const size_t szBits);
void    print_hex_par (const byte_t * pbtData, const size_t szBits, const byte_t * pbtDataPar);

void    print_nfc_iso14443a_info (const nfc_iso14443a_info_t nai);
void    print_nfc_iso14443b_info (const nfc_iso14443b_info_t nbi);
void    print_nfc_felica_info (const nfc_felica_info_t nfi);

nfc_device_desc_t *parse_device_desc (int argc, const char *argv[], size_t * szFound);

#endif
