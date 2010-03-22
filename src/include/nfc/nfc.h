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
#define _LIBNFC_H_

#include <stdint.h>
#include <stdbool.h>

#if defined (_WIN32) 
  #if defined(nfc_EXPORTS)
    #define  NFC_EXPORT __declspec(dllexport)
  #else
    #define  NFC_EXPORT __declspec(dllimport)
  #endif /* nfc_EXPORTS */
#else /* defined (_WIN32) */
 #define NFC_EXPORT
#endif

#include <nfc/nfc-types.h>

#ifdef __cplusplus 
    extern "C" {
#endif // __cplusplus

/* NFC Device/Hardware manipulation */
NFC_EXPORT void nfc_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound);
NFC_EXPORT nfc_device_t* nfc_connect(nfc_device_desc_t* pndd);
NFC_EXPORT void nfc_disconnect(nfc_device_t* pnd);
NFC_EXPORT bool nfc_configure(nfc_device_t* pnd, const nfc_device_option_t ndo, const bool bEnable);

/* NFC initiator: act as "reader" */
NFC_EXPORT bool nfc_initiator_init(const nfc_device_t* pnd);
NFC_EXPORT bool nfc_initiator_select_tag(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pti);
NFC_EXPORT bool nfc_initiator_select_dep_target(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t* pti);
NFC_EXPORT bool nfc_initiator_deselect_tag(const nfc_device_t* pnd);
NFC_EXPORT bool nfc_initiator_transceive_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);
NFC_EXPORT bool nfc_initiator_transceive_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);
NFC_EXPORT bool nfc_initiator_transceive_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);
NFC_EXPORT bool nfc_initiator_mifare_cmd(const nfc_device_t* pnd, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp);

/* NFC target: act as tag (i.e. MIFARE Classic) or NFC target device. */
NFC_EXPORT bool nfc_target_init(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits);
NFC_EXPORT bool nfc_target_receive_bits(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);
NFC_EXPORT bool nfc_target_receive_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen);
NFC_EXPORT bool nfc_target_receive_dep_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen);
NFC_EXPORT bool nfc_target_send_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar);
NFC_EXPORT bool nfc_target_send_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen);
NFC_EXPORT bool nfc_target_send_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen);

/* Special data accessors */
NFC_EXPORT const char* nfc_device_name(nfc_device_t* pnd);

/* Misc. functions */
NFC_EXPORT const char* nfc_version(void);

#ifdef __cplusplus 
}
#endif // __cplusplus


#endif // _LIBNFC_H_

