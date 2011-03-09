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

#  include <nfc/nfc-types.h>
#  include <stdbool.h>
#  include <err.h>

// TODO: Put generic errors here
#  define DENOTSUP        0x0400/* Not supported */

/**
 * @macro PRINT_HEX
 * @brief Print a byte-array in hexadecimal format (only in DEBUG mode)
 */
#  ifdef DEBUG
#    define PRINT_HEX(pcTag, pbtData, szBytes) do { \
    size_t __szPos; \
    printf(" %s: ", pcTag); \
    for (__szPos=0; __szPos < (size_t)(szBytes); __szPos++) { \
      printf("%02x  ",((uint8_t *)(pbtData))[__szPos]); \
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

/**
 * @macro DBG
 * @brief Print a message of standard output only in DEBUG mode
 */
#ifdef DEBUG
#  define DBG(...) do { \
    warnx ("DBG %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define DBG(...) {}
#endif

/**
 * @macro WARN
 * @brief Print a warn message
 */
#ifdef DEBUG
#  define WARN(...) do { \
    warnx ("WARNING %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define WARN(...) warnx ("WARNING: " __VA_ARGS__ )
#endif

/**
 * @macro ERR
 * @brief Print a error message
 */
#ifdef DEBUG
#  define ERR(...) do { \
    warnx ("ERROR %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define ERR(...)  warnx ("ERROR: " __VA_ARGS__ )
#endif

#define HAL( FUNCTION, ... ) pnd->iLastError = 0; \
  if (pnd->driver->FUNCTION) { \
    return pnd->driver->FUNCTION( __VA_ARGS__ ); \
  } else { \
    pnd->iLastError = DENOTSUP; \
    return false; \
  }

struct nfc_driver_t {
  const char *name;
  bool (*probe)(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
  nfc_device_t * (*connect)(const nfc_device_desc_t * pndd);
  void (*disconnect)(nfc_device_t * pnd);
  const char *(*strerror)(const nfc_device_t * pnd);

  bool (*initiator_init) (nfc_device_t * pnd);
  bool (*initiator_select_passive_target) (nfc_device_t * pnd,  const nfc_modulation_t nm, const byte_t * pbtInitData, const size_t szInitData, nfc_target_t * pnt);
  bool (*initiator_poll_targets) (nfc_device_t * pnd, const nfc_modulation_t * pnmModulations, const size_t szModulations, const byte_t btPollNr, const byte_t btPeriod, nfc_target_t * pntTargets, size_t * pszTargetFound);
  bool (*initiator_select_dep_target) (nfc_device_t * pnd, const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr, const nfc_dep_info_t * pndiInitiator, nfc_target_t * pnt);
  bool (*initiator_deselect_target) (nfc_device_t * pnd);
  bool (*initiator_transceive_bytes) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx, size_t * pszRx);
  bool (*initiator_transceive_bits) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);

  bool (*target_init) (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx);
  bool (*target_send_bytes) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx);
  bool (*target_receive_bytes) (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx);
  bool (*target_send_bits) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar);
  bool (*target_receive_bits) (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);

  bool (*configure) (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable);
};

nfc_device_t  *nfc_device_new (void);
void           nfc_device_free (nfc_device_t *nfc_device);


#endif // __NFC_INTERNAL_H__
