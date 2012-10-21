/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009 Roel Verdult
 * Copyright (C) 2010, 2011 Romain Tartière
 * Copyright (C) 2009, 2010, 2011, 2012 Romuald Conty
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

int     sprint_hex(char *dst, const uint8_t *pbtData, const size_t szLen);
void    sprint_nfc_iso14443a_info(char *dst, const nfc_iso14443a_info nai, bool verbose);
void    sprint_nfc_iso14443b_info(char *dst, const nfc_iso14443b_info nbi, bool verbose);
void    sprint_nfc_iso14443bi_info(char *dst, const nfc_iso14443bi_info nii, bool verbose);
void    sprint_nfc_iso14443b2sr_info(char *dst, const nfc_iso14443b2sr_info nsi, bool verbose);
void    sprint_nfc_iso14443b2ct_info(char *dst, const nfc_iso14443b2ct_info nci, bool verbose);
void    sprint_nfc_felica_info(char *dst, const nfc_felica_info nfi, bool verbose);
void    sprint_nfc_jewel_info(char *dst, const nfc_jewel_info nji, bool verbose);
void    sprint_nfc_dep_info(char *dst, const nfc_dep_info ndi, bool verbose);
void    sprint_nfc_target(char *dst, const nfc_target nt, bool verbose);

#endif
