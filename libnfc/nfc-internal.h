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
    fprintf(stderr, " %s: ", pcTag); \
    for (__szPos=0; __szPos < (size_t)(szBytes); __szPos++) { \
      fprintf(stderr, "%02x  ",((uint8_t *)(pbtData))[__szPos]); \
    } \
    fprintf(stderr, "\n"); \
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

/*
 * Buffer management macros.
 * 
 * The following macros ease setting-up and using buffers:
 * BUFFER_INIT (data, 5);      // data -> [ xx, xx, xx, xx, xx ]
 * BUFFER_SIZE (data);         // size -> 0
 * BUFFER_APPEND (data, 0x12); // data -> [ 12, xx, xx, xx, xx ]
 * BUFFER_SIZE (data);         // size -> 1
 * uint16_t x = 0x3456;        // We suppose we are little endian
 * BUFFER_APPEND_BYTES (data, x, 2);
 *                             // data -> [ 12, 56, 34, xx, xx ]
 * BUFFER_SIZE (data);         // size -> 3
 * BUFFER_APPEND_LE (data, x, 2, sizeof (x));
 *                             // data -> [ 12, 56, 34, 34, 56 ]
 * BUFFER_SIZE (data);         // size -> 5
 */

/*
 * Initialise a buffer named buffer_name of size bytes.
 */
#define BUFFER_INIT(buffer_name, size) \
    uint8_t buffer_name[size]; \
    size_t __##buffer_name##_n = 0

/*
 * Create a wrapper for an existing buffer.
 * BEWARE!  It eats children!
 */
#define BUFFER_ALIAS(buffer_name, origin) \
    uint8_t *buffer_name = (void *)origin; \
    size_t __##buffer_name##_n = 0;

#define BUFFER_SIZE(buffer_name) (__##buffer_name##_n)

#define BUFFER_CLEAR(buffer_name) (__##buffer_name##_n = 0)
/*
 * Append one byte of data to the buffer buffer_name.
 */
#define BUFFER_APPEND(buffer_name, data) \
    do { \
	buffer_name[__##buffer_name##_n++] = data; \
    } while (0)

/*
 * Append size bytes of data to the buffer buffer_name.
 */
#define BUFFER_APPEND_BYTES(buffer_name, data, size) \
    do { \
	size_t __n = 0; \
	while (__n < size) { \
            buffer_name[__##buffer_name##_n++] = ((uint8_t *)data)[__n++]; \
        } \
    } while (0)

/*
 * Append data_size bytes of data at the end of the buffer.  Since data is
 * copied as a little endian value, the storage size of the value has to be
 * passed as the field_size parameter.
 *
 * Example: to copy 24 bits of data from a 32 bits value:
 * BUFFER_APPEND_LE (buffer, data, 3, 4);
 */

#if _BYTE_ORDER != _LITTLE_ENDIAN
#define BUFFER_APPEND_LE(buffer, data, data_size, field_size) \
    do { \
        size_t __data_size = data_size; \
        size_t __field_size = field_size; \
        while (__field_size--, __data_size--) { \
            buffer[__##buffer##_n++] = ((uint8_t *)&data)[__field_size]; \
        } \
    } while (0)
#else
#define BUFFER_APPEND_LE(buffer, data, data_size, field_size) \
    do { \
        memcpy (buffer + __##buffer##_n, &data, data_size); \
        __##buffer##_n += data_size; \
    } while (0)
#endif


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
  bool (*initiator_transceive_bytes_timed) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx, size_t * pszRx, uint32_t * cycles);
  bool (*initiator_transceive_bits_timed) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar, uint32_t * cycles);

  bool (*target_init) (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx);
  bool (*target_send_bytes) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx);
  bool (*target_receive_bytes) (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx);
  bool (*target_send_bits) (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar);
  bool (*target_receive_bits) (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);

  bool (*configure) (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable);

  bool (*abort_command) (nfc_device_t * pnd);
  bool (*idle) (nfc_device_t * pnd);
};

nfc_device_t  *nfc_device_new (void);
void           nfc_device_free (nfc_device_t *nfc_device);


#endif // __NFC_INTERNAL_H__
