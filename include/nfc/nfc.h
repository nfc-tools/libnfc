/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
  NFC_EXPORT nfc_device_t *nfc_connect (nfc_device_desc_t * pndd);
  NFC_EXPORT void nfc_disconnect (nfc_device_t * pnd);
  NFC_EXPORT void nfc_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
  NFC_EXPORT bool nfc_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable);

/* NFC initiator: act as "reader" */
  NFC_EXPORT bool nfc_initiator_init (nfc_device_t * pnd);
  NFC_EXPORT bool nfc_initiator_select_passive_target (nfc_device_t * pnd, const nfc_modulation_t nm,
                                                       const byte_t * pbtInitData, const size_t szInitData,
                                                       nfc_target_t * pnt);
  NFC_EXPORT bool nfc_initiator_list_passive_targets (nfc_device_t * pnd, const nfc_modulation_t nm,
                                                      nfc_target_t ant[], const size_t szTargets,
                                                      size_t * pszTargetFound);
  NFC_EXPORT bool nfc_initiator_poll_targets (nfc_device_t * pnd, const nfc_modulation_t * pnmTargetTypes,
                                              const size_t szTargetTypes, const byte_t btPollNr, const byte_t btPeriod,
                                              nfc_target_t * pntTargets, size_t * pszTargetFound);
  NFC_EXPORT bool nfc_initiator_select_dep_target (nfc_device_t * pnd,
                                                   const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr,
                                                   const nfc_dep_info_t * pndiInitiator,
                                                   nfc_target_t * pnt);
  NFC_EXPORT bool nfc_initiator_deselect_target (nfc_device_t * pnd);
  NFC_EXPORT bool nfc_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx,
                                                  byte_t * pbtRx, size_t * pszRx);
  NFC_EXPORT bool nfc_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                                 const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits,
                                                 byte_t * pbtRxPar);

/* NFC target: act as tag (i.e. MIFARE Classic) or NFC target device. */
  NFC_EXPORT bool nfc_target_init (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx);
  NFC_EXPORT bool nfc_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx);
  NFC_EXPORT bool nfc_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx);
  NFC_EXPORT bool nfc_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                        const byte_t * pbtTxPar);
  NFC_EXPORT bool nfc_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);

/* Error reporting */
  NFC_EXPORT const char *nfc_strerror (const nfc_device_t * pnd);
  NFC_EXPORT int nfc_strerror_r (const nfc_device_t * pnd, char *pcStrErrBuf, size_t szBufLen);
  NFC_EXPORT void nfc_perror (const nfc_device_t * pnd, const char *pcString);

/* Special data accessors */
  NFC_EXPORT const char *nfc_device_name (nfc_device_t * pnd);

/* Misc. functions */
  NFC_EXPORT void iso14443a_crc (byte_t * pbtData, size_t szLen, byte_t * pbtCrc);
  NFC_EXPORT void iso14443a_crc_append (byte_t * pbtData, size_t szLen);
  NFC_EXPORT byte_t * iso14443a_locate_historical_bytes (byte_t * pbtAts, size_t szAts, size_t * pszTk);
  NFC_EXPORT const char *nfc_version (void);

/* PN53x specific errors */
// TODO: Be not PN53x-specific here
#define ETIMEOUT	0x01
#define ECRC		0x02
#define EPARITY		0x03
#define EBITCOUNT	0x04
#define EFRAMING	0x05
#define EBITCOLL	0x06
#define ESMALLBUF	0x07
#define EBUFOVF		0x09
#define ERFTIMEOUT	0x0a
#define ERFPROTO	0x0b
#define EOVHEAT		0x0d
#define EINBUFOVF	0x0e
#define EINVPARAM	0x10
#define EDEPUNKCMD	0x12
#define EINVRXFRAM	0x13
#define EMFAUTH		0x14
#define ENSECNOTSUPP	0x18	// PN533 only
#define EBCC		0x23
#define EDEPINVSTATE	0x25
#define EOPNOTALL	0x26
#define ECMD		0x27
#define ETGREL		0x29
#define ECID		0x2a
#define ECDISCARDED	0x2b
#define ENFCID3		0x2c
#define EOVCURRENT	0x2d
#define ENAD		0x2e

/* Software level errors */
#define ETGUIDNOTSUP    0x0100  /* Target UID not supported */

/* Common device-level errors */
#define DEIO            0x1000  /* Input/output error */
#define DEINVAL         0x2000  /* Invalid argument */
#define DETIMEOUT       0x3000  /* Operation timeout */

#  ifdef __cplusplus
}
#  endif                        // __cplusplus
#endif                          // _LIBNFC_H_
