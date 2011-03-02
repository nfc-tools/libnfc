/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2011, Romain Tartière, Romuald Conty
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
 * @file nfc-internal.h
 * @brief Internal defines and macros
 */

#ifndef __NFC_INTERNAL_H__
#  define __NFC_INTERNAL_H__

#  ifdef DEBUG
//#  if 1
#    define PRINT_HEX(pcTag, pbtData, szBytes) do { \
    size_t __szPos; \
    printf(" %s: ", pcTag); \
    for (__szPos=0; __szPos < (size_t)(szBytes); __szPos++) { \
      printf("%02x  ",pbtData[__szPos]); \
    } \
    printf("\n"); \
  } while (0);
#  else
#    define PRINT_HEX(pcTag, pbtData, szBytes) do { \
    (void) pcTag; \
    (void) pbtData; \
    (void) szBytes; \
  } while (0);
#  endif

struct nfc_driver_t {
  const char *name;
  bool (*probe)(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
  nfc_device_t * (*connect)(const nfc_device_desc_t * pndd);
  bool (*send)(nfc_device_t * pnd, const byte_t * pbtData, const size_t szData);
  int (*receive)(nfc_device_t * pnd, byte_t * pbtData, const size_t szDataLen);
  void (*disconnect)(nfc_device_t * pnd);
  const char *(*strerror)(const nfc_device_t * pnd);
};

#endif // __NFC_INTERNAL_H__
