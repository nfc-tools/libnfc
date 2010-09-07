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
 * @file nfc.h
 * @brief libnfc interface
 *
 * Provide all usefull functions (API) to handle NFC devices.
 */

#ifndef _LIBNFC_H_
#  define _LIBNFC_H_

#  include <stdint.h>
#  include <stdbool.h>

#  ifdef _WIN32
  /* Windows platform */
#    ifndef _WINDLL
    /* CMake compilation */
#      ifdef nfc_EXPORTS
#        define  NFC_EXPORT __declspec(dllexport)
#      else
      /* nfc_EXPORTS */
#        define  NFC_EXPORT __declspec(dllimport)
#      endif
       /* nfc_EXPORTS */
#    else
      /* _WINDLL */
    /* Manual makefile */
#      define NFC_EXPORT
#    endif
       /* _WINDLL */
#  else
      /* _WIN32 */
#    define NFC_EXPORT
#  endif
       /* _WIN32 */

#  include <nfc/nfc-types.h>

#  ifdef __cplusplus
extern  "C" {
#  endif                        // __cplusplus

/* NFC Device/Hardware manipulation */
  NFC_EXPORT void nfc_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
  NFC_EXPORT nfc_device_t *nfc_connect (nfc_device_desc_t * pndd);
  NFC_EXPORT void nfc_disconnect (nfc_device_t * pnd);
  NFC_EXPORT bool nfc_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable);

/* NFC initiator: act as "reader" */
  NFC_EXPORT bool nfc_initiator_init (nfc_device_t * pnd);
  NFC_EXPORT bool nfc_initiator_select_passive_target (nfc_device_t * pnd, const nfc_modulation_t nmInitModulation,
                                                       const byte_t * pbtInitData, const size_t szInitDataLen,
                                                       nfc_target_info_t * pti);
  NFC_EXPORT bool nfc_initiator_list_passive_targets (nfc_device_t * pnd, const nfc_modulation_t nmInitModulation,
                                                      nfc_target_info_t anti[], const size_t szTargets,
                                                      size_t * pszTargetFound);
  NFC_EXPORT bool nfc_initiator_poll_targets (nfc_device_t * pnd, const nfc_target_type_t * pnttTargetTypes,
                                              const size_t szTargetTypes, const byte_t btPollNr, const byte_t btPeriod,
                                              nfc_target_t * pntTargets, size_t * pszTargetFound);
  NFC_EXPORT bool nfc_initiator_select_dep_target (nfc_device_t * pnd, const nfc_modulation_t nmInitModulation,
                                                   const byte_t * pbtPidData, const size_t szPidDataLen,
                                                   const byte_t * pbtNFCID3i, const size_t szNFCID3iDataLen,
                                                   const byte_t * pbtGbData, const size_t szGbDataLen,
                                                   nfc_target_info_t * pti);
  NFC_EXPORT bool nfc_initiator_deselect_target (nfc_device_t * pnd);
  NFC_EXPORT bool nfc_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                                 const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits,
                                                 byte_t * pbtRxPar);
  NFC_EXPORT bool nfc_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxLen,
                                                  byte_t * pbtRx, size_t * pszRxLen);

/* NFC target: act as tag (i.e. MIFARE Classic) or NFC target device. */
  NFC_EXPORT bool nfc_target_init (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits);
  NFC_EXPORT bool nfc_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);
  NFC_EXPORT bool nfc_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxLen);
  NFC_EXPORT bool nfc_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                        const byte_t * pbtTxPar);
  NFC_EXPORT bool nfc_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxLen);

/* Error reporting */
  NFC_EXPORT const char *nfc_strerror (const nfc_device_t * pnd);
  NFC_EXPORT int nfc_strerror_r (const nfc_device_t * pnd, char *pcStrErrBuf, size_t szBufLen);
  NFC_EXPORT void nfc_perror (const nfc_device_t * pnd, const char *pcString);

/* Special data accessors */
  NFC_EXPORT const char *nfc_device_name (nfc_device_t * pnd);

/* Misc. functions */
  NFC_EXPORT void iso14443a_crc (byte_t * pbtData, size_t szLen, byte_t * pbtCrc);
  NFC_EXPORT void append_iso14443a_crc (byte_t * pbtData, size_t szLen);
  NFC_EXPORT const char *nfc_version (void);

/* Common device-level errors */
#  define DEIO            0x1000/* Input/output error */
#  define DEINVAL         0x2000/* Invalid argument */
#  define DETIMEOUT       0x3000/* Operation timeout */

#  ifdef __cplusplus
}
#  endif                        // __cplusplus
#endif                          // _LIBNFC_H_
