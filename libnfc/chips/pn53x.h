/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult, Romuald Conty
 * Copyright (C) 2010, Roel Verdult, Romuald Conty, Romain Tartière
 * Copyright (C) 2011, Romuald Conty, Romain Tartière
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
 * @file pn53x.h
 * @brief PN531, PN532 and PN533 common functions
 */

#ifndef __NFC_CHIPS_PN53X_H__
#  define __NFC_CHIPS_PN53X_H__

#  include <nfc/nfc-types.h>
#  include "pn53x-internal.h"

// Registers and symbols masks used to covers parts within a register
//   PN53X_REG_CIU_TxMode
#  define SYMBOL_TX_CRC_ENABLE      0x80
#  define SYMBOL_TX_SPEED           0x70
// TX_FRAMING bits explanation:
//   00 : ISO/IEC 14443A/MIFARE and Passive Communication mode 106 kbit/s
//   01 : Active Communication mode
//   10 : FeliCa and Passive Communication mode at 212 kbit/s and 424 kbit/s
//   11 : ISO/IEC 14443B
#  define SYMBOL_TX_FRAMING         0x03

//   PN53X_REG_Control_switch_rng
#  define SYMBOL_CURLIMOFF          0x08     /* When set to 1, the 100 mA current limitations is desactivated. */
#  define SYMBOL_SIC_SWITCH_EN      0x10     /* When set to logic 1, the SVDD switch is enabled and the SVDD output delivers power to secure IC and internal pads (SIGIN, SIGOUT and P34). */
#  define SYMBOL_RANDOM_DATAREADY   0x02     /* When set to logic 1, a new random number is available. */

//   PN53X_REG_CIU_RxMode
#  define SYMBOL_RX_CRC_ENABLE      0x80
#  define SYMBOL_RX_SPEED           0x70
#  define SYMBOL_RX_NO_ERROR        0x08
#  define SYMBOL_RX_MULTIPLE        0x04
// RX_FRAMING follow same scheme than TX_FRAMING
#  define SYMBOL_RX_FRAMING         0x03

//   PN53X_REG_CIU_TxAuto
#  define SYMBOL_FORCE_100_ASK      0x40
#  define SYMBOL_AUTO_WAKE_UP       0x20
#  define SYMBOL_INITIAL_RF_ON      0x04

//   PN53X_REG_CIU_ManualRCV
#  define SYMBOL_PARITY_DISABLE     0x10

//   PN53X_REG_CIU_TMode
#  define SYMBOL_TAUTO              0x80
#  define SYMBOL_TPRESCALERHI       0x0F

//   PN53X_REG_CIU_TPrescaler
#  define SYMBOL_TPRESCALERLO       0xFF

//   PN53X_REG_CIU_Command
#  define SYMBOL_COMMAND            0x0F
#  define SYMBOL_COMMAND_TRANSCEIVE 0xC

//   PN53X_REG_CIU_Status2
#  define SYMBOL_MF_CRYPTO1_ON      0x08

//   PN53X_REG_CIU_FIFOLevel
#  define SYMBOL_FLUSH_BUFFER       0x80
#  define SYMBOL_FIFO_LEVEL         0x7F

//   PN53X_REG_CIU_Control
#  define SYMBOL_INITIATOR          0x10
#  define SYMBOL_RX_LAST_BITS       0x07

//   PN53X_REG_CIU_BitFraming
#  define SYMBOL_START_SEND         0x80
#  define SYMBOL_RX_ALIGN           0x70
#  define SYMBOL_TX_LAST_BITS       0x07

// PN53X Support Byte flags
#define SUPPORT_ISO14443A             0x01
#define SUPPORT_ISO14443B             0x02
#define SUPPORT_ISO18092              0x04

// Internal parameters flags
#  define PARAM_NONE                  0x00
#  define PARAM_NAD_USED              0x01
#  define PARAM_DID_USED              0x02
#  define PARAM_AUTO_ATR_RES          0x04
#  define PARAM_AUTO_RATS             0x10
#  define PARAM_14443_4_PICC          0x20 /* Only for PN532 */
#  define PARAM_NFC_SECURE            0x20 /* Only for PN533 */
#  define PARAM_NO_AMBLE              0x40 /* Only for PN532 */

// Radio Field Configure Items           // Configuration Data length
#  define RFCI_FIELD                  0x01      //  1
#  define RFCI_TIMING                 0x02      //  3
#  define RFCI_RETRY_DATA             0x04      //  1
#  define RFCI_RETRY_SELECT           0x05      //  3
#  define RFCI_ANALOG_TYPE_A_106      0x0A      // 11
#  define RFCI_ANALOG_TYPE_A_212_424  0x0B      //  8
#  define RFCI_ANALOG_TYPE_B          0x0C      //  3
#  define RFCI_ANALOG_TYPE_14443_4    0x0D      //  9

/* PN53x specific device-level errors */
#  define DENACK          0x0100/* NACK */
#  define DEACKMISMATCH   0x0200/* Unexpected data */
#  define DEISERRFRAME    0x0300/* Error frame */

typedef enum {
  NORMAL,	// In that case, there is no power saved but the PN53x reacts as fast as possible on the host controller interface.
  POWERDOWN,	// Only on PN532, need to be wake up to process commands with a long preamble
  LOWVBAT	// Only on PN532, need to be wake up to process commands with a long preamble and SAMConfiguration command
} pn53x_power_mode;

typedef enum {
  IDLE,
  INITIATOR,
  TARGET,
} pn53x_operating_mode;

struct pn53x_io {
  bool (*send)(nfc_device_t * pnd, const byte_t * pbtData, const size_t szData);
  int (*receive)(nfc_device_t * pnd, byte_t * pbtData, const size_t szDataLen);
};

/* defines */
#define PN53X_CACHE_REGISTER_MIN_ADDRESS 	PN53X_REG_CIU_Mode
#define PN53X_CACHE_REGISTER_MAX_ADDRESS 	PN53X_REG_CIU_Coll
#define PN53X_CACHE_REGISTER_SIZE 		((PN53X_CACHE_REGISTER_MAX_ADDRESS - PN53X_CACHE_REGISTER_MIN_ADDRESS) + 1)
struct pn53x_data {
/** Chip type (PN531, PN532 or PN533)*/
  pn53x_type type;
/** Current power mode */
  pn53x_power_mode power_mode;
/** Current operating mode */
  pn53x_operating_mode operating_mode;
/** Current emulated target */
  nfc_target_t* current_target;
/** PN53x I/O functions stored in struct */
  const struct pn53x_io * io;
/** Register cache for REG_CIU_BIT_FRAMING, SYMBOL_TX_LAST_BITS: The last TX bits setting, we need to reset this if it does not apply anymore */
  uint8_t ui8TxBits;
/** Register cache for SetParameters function. */
  uint8_t ui8Parameters;
/** Last sent command */
  uint8_t ui8LastCommand;
/** Interframe timer correction */
  int16_t timer_correction;
/** Timer prescaler */
  uint16_t timer_prescaler;
/** WriteBack cache */
  uint8_t wb_data[PN53X_CACHE_REGISTER_SIZE];
  uint8_t wb_mask[PN53X_CACHE_REGISTER_SIZE];
  bool wb_trigged;
};

#define CHIP_DATA(pnd) ((struct pn53x_data*)(pnd->chip_data))

/**
 * @enum pn53x_modulation_t
 * @brief NFC modulation
 */
typedef enum {
  /** Undefined modulation */
  PM_UNDEFINED = -1,
/** ISO14443-A (NXP MIFARE) http://en.wikipedia.org/wiki/MIFARE */
  PM_ISO14443A_106 = 0x00,
/** JIS X 6319-4 (Sony Felica) http://en.wikipedia.org/wiki/FeliCa */
  PM_FELICA_212 = 0x01,
/** JIS X 6319-4 (Sony Felica) http://en.wikipedia.org/wiki/FeliCa */
  PM_FELICA_424 = 0x02,
/** ISO14443-B http://en.wikipedia.org/wiki/ISO/IEC_14443 (Not supported by PN531) */
  PM_ISO14443B_106 = 0x03, 
/** Jewel Topaz (Innovision Research & Development) (Not supported by PN531) */
  PM_JEWEL_106 = 0x04,
/** ISO14443-B http://en.wikipedia.org/wiki/ISO/IEC_14443 (Not supported by PN531 nor PN532) */
  PM_ISO14443B_212 = 0x06,
/** ISO14443-B http://en.wikipedia.org/wiki/ISO/IEC_14443 (Not supported by PN531 nor PN532) */
  PM_ISO14443B_424 = 0x07,
/** ISO14443-B http://en.wikipedia.org/wiki/ISO/IEC_14443 (Not supported by PN531 nor PN532) */
  PM_ISO14443B_847 = 0x08,
} pn53x_modulation_t;

/**
 * @enum pn53x_target_type_t
 * @brief NFC target type enumeration
 */
typedef enum {
  /** Undefined target type */
  PTT_UNDEFINED = -1,
  /** Generic passive 106 kbps (ISO/IEC14443-4A, mifare, DEP) */
  PTT_GENERIC_PASSIVE_106 = 0x00,
  /** Generic passive 212 kbps (FeliCa, DEP) */
  PTT_GENERIC_PASSIVE_212 = 0x01,
  /** Generic passive 424 kbps (FeliCa, DEP) */
  PTT_GENERIC_PASSIVE_424 = 0x02,
  /** Passive 106 kbps ISO/IEC14443-4B */
  PTT_ISO14443_4B_106 = 0x03,
  /** Innovision Jewel tag */
  PTT_JEWEL_106 = 0x04,
  /** Mifare card */
  PTT_MIFARE = 0x10,
  /** FeliCa 212 kbps card */
  PTT_FELICA_212 = 0x11,
  /** FeliCa 424 kbps card */
  PTT_FELICA_424 = 0x12,
  /** Passive 106 kbps ISO/IEC 14443-4A */
  PTT_ISO14443_4A_106 = 0x20,
  /** Passive 106 kbps ISO/IEC 14443-4B with TCL flag */
  PTT_ISO14443_4B_TCL_106 = 0x23,
  /** DEP passive 106 kbps */
  PTT_DEP_PASSIVE_106 = 0x40,
  /** DEP passive 212 kbps */
  PTT_DEP_PASSIVE_212 = 0x41,
  /** DEP passive 424 kbps */
  PTT_DEP_PASSIVE_424 = 0x42,
  /** DEP active 106 kbps */
  PTT_DEP_ACTIVE_106 = 0x80,
  /** DEP active 212 kbps */
  PTT_DEP_ACTIVE_212 = 0x81,
  /** DEP active 424 kbps */
  PTT_DEP_ACTIVE_424 = 0x82,
} pn53x_target_type_t;

/**
 * @enum pn53x_target_mode_t
 * @brief PN53x target mode enumeration
 */
typedef enum {
  /** Configure the PN53x to accept all initiator mode */
  PTM_NORMAL = 0x00,
  /** Configure the PN53x to accept to be initialized only in passive mode */
  PTM_PASSIVE_ONLY = 0x01,
  /** Configure the PN53x to accept to be initialized only as DEP target */
  PTM_DEP_ONLY = 0x02,
  /** Configure the PN532 to accept to be initialized only as ISO/IEC14443-4 PICC */
  PTM_ISO14443_4_PICC_ONLY = 0x04
} pn53x_target_mode_t;

bool    pn53x_init(nfc_device_t * pnd);
bool    pn53x_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx, size_t *pszRx);

bool    pn53x_set_parameters (nfc_device_t * pnd, const uint8_t ui8Value, const bool bEnable);
bool    pn53x_set_tx_bits (nfc_device_t * pnd, const uint8_t ui8Bits);
bool    pn53x_wrap_frame (const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar, byte_t * pbtFrame,
                          size_t * pszFrameBits);
bool    pn53x_unwrap_frame (const byte_t * pbtFrame, const size_t szFrameBits, byte_t * pbtRx, size_t * pszRxBits,
                            byte_t * pbtRxPar);
bool    pn53x_decode_target_data (const byte_t * pbtRawData, size_t szRawData,
                                  pn53x_type chip_type, nfc_modulation_type_t nmt,
                                  nfc_target_info_t * pnti);
bool    pn53x_read_register (nfc_device_t * pnd, uint16_t ui16Reg, uint8_t * ui8Value);
bool    pn53x_write_register (nfc_device_t * pnd, uint16_t ui16Reg, uint8_t ui8SymbolMask, uint8_t ui8Value);
bool    pn53x_get_firmware_version (nfc_device_t * pnd, char abtFirmwareText[18]);
bool    pn53x_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable);
bool    pn53x_check_communication (nfc_device_t *pnd);
bool    pn53x_idle (nfc_device_t * pnd);

// NFC device as Initiator functions
bool    pn53x_initiator_init (nfc_device_t * pnd);
bool    pn53x_initiator_select_passive_target (nfc_device_t * pnd,
                                               const nfc_modulation_t nm,
                                               const byte_t * pbtInitData, const size_t szInitData,
                                               nfc_target_t * pnt);
bool    pn53x_initiator_poll_targets (nfc_device_t * pnd,
                                      const nfc_modulation_t * pnmModulations, const size_t szModulations,
                                      const byte_t btPollNr, const byte_t btPeriod,
                                      nfc_target_t * pntTargets, size_t * pszTargetFound);
bool    pn53x_initiator_select_dep_target (nfc_device_t * pnd,
                                           const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr,
                                           const nfc_dep_info_t * pndiInitiator, 
                                           nfc_target_t * pnt);
bool    pn53x_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                         const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits,
                                         byte_t * pbtRxPar);
bool    pn53x_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx,
                                          byte_t * pbtRx, size_t * pszRx);
bool    pn53x_initiator_transceive_bits_timed (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                         const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits,
                                         byte_t * pbtRxPar, uint32_t * cycles);
bool    pn53x_initiator_transceive_bytes_timed (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx,
                                          byte_t * pbtRx, size_t * pszRx, uint32_t * cycles);
bool    pn53x_initiator_deselect_target (nfc_device_t * pnd);

// NFC device as Target functions
bool    pn53x_target_init (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx);
bool    pn53x_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar);
bool    pn53x_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx);
bool    pn53x_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                const byte_t * pbtTxPar);
bool    pn53x_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx);

// Error handling functions
const char *pn53x_strerror (const nfc_device_t * pnd);

// C wrappers for PN53x commands
bool    pn53x_SetParameters (nfc_device_t * pnd, const uint8_t ui8Value);
bool    pn53x_SAMConfiguration (nfc_device_t * pnd, const uint8_t ui8Mode);
bool    pn53x_PowerDown (nfc_device_t * pnd);
bool    pn53x_InListPassiveTarget (nfc_device_t * pnd, const pn53x_modulation_t pmInitModulation,
                                   const byte_t szMaxTargets, const byte_t * pbtInitiatorData,
                                   const size_t szInitiatorDataLen, byte_t * pbtTargetsData, size_t * pszTargetsData);
bool    pn53x_InDeselect (nfc_device_t * pnd, const uint8_t ui8Target);
bool    pn53x_InRelease (nfc_device_t * pnd, const uint8_t ui8Target);
bool    pn53x_InAutoPoll (nfc_device_t * pnd, const pn53x_target_type_t * ppttTargetTypes, const size_t szTargetTypes,
                          const byte_t btPollNr, const byte_t btPeriod, nfc_target_t * pntTargets,
                          size_t * pszTargetFound);
bool    pn53x_InJumpForDEP (nfc_device_t * pnd,
                            const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr,
                            const byte_t * pbtPassiveInitiatorData,
                            const byte_t * pbtNFCID3i,
                            const byte_t * pbtGB, const size_t szGB,
                            nfc_target_t * pnt);
bool	pn53x_TgInitAsTarget (nfc_device_t * pnd, pn53x_target_mode_t ptm,
                              const byte_t * pbtMifareParams,
                              const byte_t * pbtTkt, size_t szTkt,
                              const byte_t * pbtFeliCaParams,
                              const byte_t * pbtNFCID3t, const byte_t * pbtGB, const size_t szGB,
                              byte_t * pbtRx, size_t * pszRx, byte_t * pbtModeByte);

// Misc
bool    pn53x_check_ack_frame (nfc_device_t * pnd, const byte_t * pbtRxFrame, const size_t szRxFrameLen);
bool    pn53x_check_error_frame (nfc_device_t * pnd, const byte_t * pbtRxFrame, const size_t szRxFrameLen);
bool    pn53x_build_frame (byte_t * pbtFrame, size_t * pszFrame, const byte_t * pbtData, const size_t szData);

void    pn53x_data_new (nfc_device_t * pnd, const struct pn53x_io* io);
void    pn53x_data_free (nfc_device_t * pnd);

#endif // __NFC_CHIPS_PN53X_H__
