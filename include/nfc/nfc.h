/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
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
 * @file nfc.h
 * @brief libnfc interface
 *
 * Provide all useful functions (API) to handle NFC devices.
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

#  ifndef __has_attribute
#    define __has_attribute(x) 0
#  endif

#  if __has_attribute(nonnull) || defined(__GNUC__)
#    define __has_attribute_nonnull 1
#  endif

#  if __has_attribute_nonnull
#    define ATTRIBUTE_NONNULL( param ) __attribute__((nonnull (param)))
#  else
#  define ATTRIBUTE_NONNULL( param )
#  endif

#  ifdef __cplusplus
extern  "C" {
#  endif                        // __cplusplus

/* Library initialization/deinitialization */
NFC_EXPORT void nfc_init(nfc_context **context) ATTRIBUTE_NONNULL(1);
NFC_EXPORT void nfc_exit(nfc_context *context) ATTRIBUTE_NONNULL(1);
NFC_EXPORT int nfc_register_driver(const nfc_driver *driver);

/* NFC Device/Hardware manipulation */
NFC_EXPORT nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring) ATTRIBUTE_NONNULL(1);
NFC_EXPORT void nfc_close(nfc_device *pnd);
NFC_EXPORT int nfc_abort_command(nfc_device *pnd);
NFC_EXPORT size_t nfc_list_devices(nfc_context *context, nfc_connstring connstrings[], size_t connstrings_len) ATTRIBUTE_NONNULL(1);
NFC_EXPORT int nfc_idle(nfc_device *pnd);

/* NFC initiator: act as "reader" */
NFC_EXPORT int nfc_initiator_init(nfc_device *pnd);
NFC_EXPORT int nfc_initiator_init_secure_element(nfc_device *pnd);
NFC_EXPORT int nfc_initiator_select_passive_target(nfc_device *pnd, const nfc_modulation nm, const uint8_t *pbtInitData, const size_t szInitData, nfc_target *pnt);
NFC_EXPORT int nfc_initiator_list_passive_targets(nfc_device *pnd, const nfc_modulation nm, nfc_target ant[], const size_t szTargets);
NFC_EXPORT int nfc_initiator_poll_target(nfc_device *pnd, const nfc_modulation *pnmTargetTypes, const size_t szTargetTypes, const uint8_t uiPollNr, const uint8_t uiPeriod, nfc_target *pnt);
NFC_EXPORT int nfc_initiator_select_dep_target(nfc_device *pnd, const nfc_dep_mode ndm, const nfc_baud_rate nbr, const nfc_dep_info *pndiInitiator, nfc_target *pnt, const int timeout);
NFC_EXPORT int nfc_initiator_poll_dep_target(nfc_device *pnd, const nfc_dep_mode ndm, const nfc_baud_rate nbr, const nfc_dep_info *pndiInitiator, nfc_target *pnt, const int timeout);
NFC_EXPORT int nfc_initiator_deselect_target(nfc_device *pnd);
NFC_EXPORT int nfc_initiator_transceive_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, int timeout);
NFC_EXPORT int nfc_initiator_transceive_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, const size_t szRx, uint8_t *pbtRxPar);
NFC_EXPORT int nfc_initiator_transceive_bytes_timed(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, uint32_t *cycles);
NFC_EXPORT int nfc_initiator_transceive_bits_timed(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, const size_t szRx, uint8_t *pbtRxPar, uint32_t *cycles);
NFC_EXPORT int nfc_initiator_target_is_present(nfc_device *pnd, const nfc_target *pnt);

/* NFC target: act as tag (i.e. MIFARE Classic) or NFC target device. */
NFC_EXPORT int nfc_target_init(nfc_device *pnd, nfc_target *pnt, uint8_t *pbtRx, const size_t szRx, int timeout);
NFC_EXPORT int nfc_target_send_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, int timeout);
NFC_EXPORT int nfc_target_receive_bytes(nfc_device *pnd, uint8_t *pbtRx, const size_t szRx, int timeout);
NFC_EXPORT int nfc_target_send_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar);
NFC_EXPORT int nfc_target_receive_bits(nfc_device *pnd, uint8_t *pbtRx, const size_t szRx, uint8_t *pbtRxPar);

/* Error reporting */
NFC_EXPORT const char *nfc_strerror(const nfc_device *pnd);
NFC_EXPORT int nfc_strerror_r(const nfc_device *pnd, char *buf, size_t buflen);
NFC_EXPORT void nfc_perror(const nfc_device *pnd, const char *s);
NFC_EXPORT int nfc_device_get_last_error(const nfc_device *pnd);

/* Special data accessors */
NFC_EXPORT const char *nfc_device_get_name(nfc_device *pnd);
NFC_EXPORT const char *nfc_device_get_connstring(nfc_device *pnd);
NFC_EXPORT int nfc_device_get_supported_modulation(nfc_device *pnd, const nfc_mode mode,  const nfc_modulation_type **const supported_mt);
NFC_EXPORT int nfc_device_get_supported_baud_rate(nfc_device *pnd, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br);
NFC_EXPORT int nfc_device_get_supported_baud_rate_target_mode(nfc_device *pnd, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br);

/* Properties accessors */
NFC_EXPORT int nfc_device_set_property_int(nfc_device *pnd, const nfc_property property, const int value);
NFC_EXPORT int nfc_device_set_property_bool(nfc_device *pnd, const nfc_property property, const bool bEnable);

/* Misc. functions */
NFC_EXPORT void iso14443a_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc);
NFC_EXPORT void iso14443a_crc_append(uint8_t *pbtData, size_t szLen);
NFC_EXPORT void iso14443b_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc);
NFC_EXPORT void iso14443b_crc_append(uint8_t *pbtData, size_t szLen);
NFC_EXPORT uint8_t *iso14443a_locate_historical_bytes(uint8_t *pbtAts, size_t szAts, size_t *pszTk);

NFC_EXPORT void nfc_free(void *p);
NFC_EXPORT const char *nfc_version(void);
NFC_EXPORT int nfc_device_get_information_about(nfc_device *pnd, char **buf);

/* String converter functions */
NFC_EXPORT const char *str_nfc_modulation_type(const nfc_modulation_type nmt);
NFC_EXPORT const char *str_nfc_baud_rate(const nfc_baud_rate nbr);
NFC_EXPORT int str_nfc_target(char **buf, const nfc_target *pnt, bool verbose);

/* Error codes */
/** @ingroup error
 * @hideinitializer
 * Success (no error)
 */
#define NFC_SUCCESS			 0
/** @ingroup error
 * @hideinitializer
 * Input / output error, device may not be usable anymore without re-open it
 */
#define NFC_EIO				-1
/** @ingroup error
 * @hideinitializer
 * Invalid argument(s)
 */
#define NFC_EINVARG			-2
/** @ingroup error
 * @hideinitializer
 *  Operation not supported by device
 */
#define NFC_EDEVNOTSUPP			-3
/** @ingroup error
 * @hideinitializer
 * No such device
 */
#define NFC_ENOTSUCHDEV			-4
/** @ingroup error
 * @hideinitializer
 * Buffer overflow
 */
#define NFC_EOVFLOW			-5
/** @ingroup error
 * @hideinitializer
 * Operation timed out
 */
#define NFC_ETIMEOUT			-6
/** @ingroup error
 * @hideinitializer
 * Operation aborted (by user)
 */
#define NFC_EOPABORTED			-7
/** @ingroup error
 * @hideinitializer
 * Not (yet) implemented
 */
#define NFC_ENOTIMPL			-8
/** @ingroup error
 * @hideinitializer
 * Target released
 */
#define NFC_ETGRELEASED			-10
/** @ingroup error
 * @hideinitializer
 * Error while RF transmission
 */
#define NFC_ERFTRANS			-20
/** @ingroup error
 * @hideinitializer
 * MIFARE Classic: authentication failed
 */
#define NFC_EMFCAUTHFAIL		-30
/** @ingroup error
 * @hideinitializer
 * Software error (allocation, file/pipe creation, etc.)
 */
#define NFC_ESOFT			-80
/** @ingroup error
 * @hideinitializer
 * Device's internal chip error
 */
#define NFC_ECHIP			-90


#  ifdef __cplusplus
}
#  endif                        // __cplusplus
#endif                          // _LIBNFC_H_
