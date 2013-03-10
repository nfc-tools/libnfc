/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Additional contributors of this file:
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
 * @file target-subr.c
 * @brief Target-related subroutines. (ie. determine target type, print target, etc.)
 */

#ifndef _TARGET_SUBR_H_
#define _TARGET_SUBR_H_

int     snprint_hex(char *dst, size_t size, const uint8_t *pbtData, const size_t szLen);
void    snprint_nfc_iso14443a_info(char *dst, size_t size, const nfc_iso14443a_info *pnai, bool verbose);
void    snprint_nfc_iso14443b_info(char *dst, size_t size, const nfc_iso14443b_info *pnbi, bool verbose);
void    snprint_nfc_iso14443bi_info(char *dst, size_t size, const nfc_iso14443bi_info *pnii, bool verbose);
void    snprint_nfc_iso14443b2sr_info(char *dst, size_t size, const nfc_iso14443b2sr_info *pnsi, bool verbose);
void    snprint_nfc_iso14443b2ct_info(char *dst, size_t size, const nfc_iso14443b2ct_info *pnci, bool verbose);
void    snprint_nfc_felica_info(char *dst, size_t size, const nfc_felica_info *pnfi, bool verbose);
void    snprint_nfc_jewel_info(char *dst, size_t size, const nfc_jewel_info *pnji, bool verbose);
void    snprint_nfc_dep_info(char *dst, size_t size, const nfc_dep_info *pndi, bool verbose);
void    snprint_nfc_target(char *dst, size_t size, const nfc_target *pnt, bool verbose);

#endif
