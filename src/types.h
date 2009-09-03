/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#ifndef _LIBNFC_TYPES_H_
#define _LIBNFC_TYPES_H_

/**
 * @file types.h
 * @brief libnfc-defined types
 *
 * Define libnfc specific types: typedef, enum, struct, etc.
 */

#include <stdint.h>
#include <stdbool.h>

#include "defines.h"

typedef uint8_t byte_t;

// Compiler directive, set struct alignment to 1 byte_t for compatibility
#pragma pack(1)

typedef enum {
  CT_PN531                    = 0x10,
  CT_PN532                    = 0x20,
  CT_PN533                    = 0x30,
} chip_type;

struct dev_callbacks;                // Prototype the callback struct

/**
 * @struct dev_info
 * @brief NFC device information
 */
typedef struct {
/** Callback functions for handling device specific wrapping */
  const struct dev_callbacks* pdc;
/** Device name string, including device wrapper firmware */
  char acName[DEVICE_NAME_LENGTH];
/** PN53X chip type, this is useful for some "bug" work-arounds */
  chip_type ct;
/** Pointer to the device connection specification */
  dev_spec ds;
/** This represents if the PN53X device was initialized succesful */
  bool bActive;
/** Is the crc automaticly added, checked and removed from the frames */
  bool bCrc;
/** Does the PN53x chip handles parity bits, all parities are handled as data */
  bool bPar;
/** The last tx bits setting, we need to reset this if it does not apply anymore */
  uint8_t ui8TxBits;
} dev_info;

/**
 * @struct dev_callbacks
 * @brief NFC defice callbacks
 */
struct dev_callbacks {
  /** Driver name */
  const char* acDriver;
  /** Connect callback */
  dev_info* (*connect)(const uint32_t uiIndex);
  /** Transceive callback */
  bool (*transceive)(const dev_spec ds, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen);
  /** Disconnect callback */
  void (*disconnect)(dev_info* pdi);
};

/**
 * @enum dev_config_option
 * @brief NFC device option
 */
typedef enum {
/** Let the PN53X chip handle the CRC bytes. This means that the chip appends the CRC bytes to the frames that are transmitted. It will parse the last bytes from received frames as incoming CRC bytes. They will be verified against the used modulation and protocol. If an frame is expected with incorrect CRC bytes this option should be disabled. Example frames where this is useful are the ATQA and UID+BCC that are transmitted without CRC bytes during the anti-collision phase of the ISO14443-A protocol. */
  DCO_HANDLE_CRC              = 0x00,
/** Parity bits in the network layer of ISO14443-A are by default generated and validated in the PN53X chip. This is a very convenient feature. On certain times though it is useful to get full control of the transmitted data. The proprietary MIFARE Classic protocol uses for example custom (encrypted) parity bits. For interoperability it is required to be completely compatible, including the arbitrary parity bits. When this option is disabled, the functions to communicating bits should be used. */
  DCO_HANDLE_PARITY           = 0x01,
/** This option can be used to enable or disable the electronic field of the NFC device. */
  DCO_ACTIVATE_FIELD          = 0x10,
/** The internal CRYPTO1 co-processor can be used to transmit messages encrypted. This option is automatically activated after a successful MIFARE Classic authentication. */
  DCO_ACTIVATE_CRYPTO1        = 0x11,
/** The default configuration defines that the PN53X chip will try indefinitely to invite a tag in the field to respond. This could be desired when it is certain a tag will enter the field. On the other hand, when this is uncertain, it will block the application. This option could best be compared to the (NON)BLOCKING option used by (socket)network programming. */
  DCO_INFINITE_SELECT         = 0x20,
/** If this option is enabled, frames that carry less than 4 bits are allowed. According to the standards these frames should normally be handles as invalid frames. */
  DCO_ACCEPT_INVALID_FRAMES   = 0x30,
/** If the NFC device should only listen to frames, it could be useful to let it gather multiple frames in a sequence. They will be stored in the internal FIFO of the PN53X chip. This could be retrieved by using the receive data functions. Note that if the chip runs out of bytes (FIFO = 64 bytes long), it will overwrite the first received frames, so quick retrieving of the received data is desirable. */
  DCO_ACCEPT_MULTIPLE_FRAMES  = 0x31
}dev_config_option;

////////////////////////////////////////////////////////////////////
// nfc_reader_list_passive - using InListPassiveTarget 

/**
 * @enum init_modulation
 * @brief NFC modulation
 */
typedef enum {
/** ISO14443-A (NXP MIFARE) http://en.wikipedia.org/wiki/MIFARE */
  IM_ISO14443A_106  = 0x00,
/** JIS X 6319-4 (Sony Felica) http://en.wikipedia.org/wiki/FeliCa */
  IM_FELICA_212     = 0x01,
/** JIS X 6319-4 (Sony Felica) http://en.wikipedia.org/wiki/FeliCa */
  IM_FELICA_424     = 0x02,
/** ISO14443-B http://en.wikipedia.org/wiki/ISO/IEC_14443 */
  IM_ISO14443B_106  = 0x03,
/** Jewel Topaz (Innovision Research & Development) */
  IM_JEWEL_106      = 0x04,
/** Active DEP */
  IM_ACTIVE_DEP = 0x05,
/** Passive DEP */
  IM_PASSIVE_DEP = 0x06,


}init_modulation;

typedef struct {
  byte_t NFCID3i[10];
  byte_t btDID;
  byte_t btBSt;
  byte_t btBRt;
}tag_info_dep;

typedef struct {
  byte_t abtAtqa[2];
  byte_t btSak;
  uint32_t uiUidLen;
  byte_t abtUid[10];
  uint32_t uiAtsLen;
  byte_t abtAts[36];
}tag_info_iso14443a;

typedef struct {
  uint32_t uiLen;
  byte_t btResCode;
  byte_t abtId[8];
  byte_t abtPad[8];
  byte_t abtSysCode[2];
}tag_info_felica;

typedef struct {
  byte_t abtAtqb[12];
  byte_t abtId[4];
  byte_t btParam1;
  byte_t btParam2;
  byte_t btParam3;
  byte_t btParam4;
  byte_t btCid;
  uint32_t uiInfLen;
  byte_t abtInf[64];
}tag_info_iso14443b;

typedef struct {
  byte_t btSensRes[2];
  byte_t btId[4];
}tag_info_jewel;

typedef union {
  tag_info_iso14443a tia;
  tag_info_felica tif;
  tag_info_iso14443b tib;
  tag_info_jewel tij;
  tag_info_dep tid;
}tag_info;

////////////////////////////////////////////////////////////////////
// InDataExchange, MIFARE Classic card 

typedef enum {
  MC_AUTH_A         = 0x60,
  MC_AUTH_B         = 0x61,
  MC_READ           = 0x30,
  MC_WRITE          = 0xA0,
  MC_TRANSFER       = 0xB0,
  MC_DECREMENT      = 0xC0,
  MC_INCREMENT      = 0xC1,
  MC_STORE          = 0xC2,
}mifare_cmd;

// MIFARE Classic command params
typedef struct {
  byte_t abtKey[6];
  byte_t abtUid[4];
}mifare_param_auth;

typedef struct {
  byte_t abtData[16];
}mifare_param_data;

typedef struct {
  byte_t abtValue[4];
}mifare_param_value;

typedef union {
  mifare_param_auth mpa;
  mifare_param_data mpd;
  mifare_param_value mpv;
}mifare_param;

// Reset struct alignment to default
#pragma pack()

#endif // _LIBNFC_TYPES_H_
