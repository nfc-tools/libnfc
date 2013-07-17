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

/**
 * @enum pn53x_power_mode
 * @brief PN53x power mode enumeration
 */
typedef enum {
  NORMAL,	// In that case, there is no power saved but the PN53x reacts as fast as possible on the host controller interface.
  POWERDOWN,	// Only on PN532, need to be wake up to process commands with a long preamble
  LOWVBAT	// Only on PN532, need to be wake up to process commands with a long preamble and SAMConfiguration command
} pn53x_power_mode;

/**
 * @enum pn53x_operating_mode
 * @brief PN53x operatin mode enumeration
 */
typedef enum {
  IDLE,
  INITIATOR,
  TARGET,
} pn53x_operating_mode;

/**
 * @enum pn532_sam_mode
 * @brief PN532 SAM mode enumeration
 */
typedef enum {
  PSM_NORMAL = 0x01,
  PSM_VIRTUAL_CARD = 0x02,
  PSM_WIRED_CARD = 0x03,
  PSM_DUAL_CARD = 0x04
} pn532_sam_mode;

/**
 * @internal
 * @struct pn53x_io
 * @brief PN53x I/O structure
 */
struct pn53x_io {
  int (*send)(struct nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout);
  int (*receive)(struct nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout);
};

/* defines */
#define PN53X_CACHE_REGISTER_MIN_ADDRESS 	PN53X_REG_CIU_Mode
#define PN53X_CACHE_REGISTER_MAX_ADDRESS 	PN53X_REG_CIU_Coll
#define PN53X_CACHE_REGISTER_SIZE 		((PN53X_CACHE_REGISTER_MAX_ADDRESS - PN53X_CACHE_REGISTER_MIN_ADDRESS) + 1)

/**
 * @internal
 * @struct pn53x_data
 * @brief PN53x data structure
 */
struct pn53x_data {
  /** Chip type (PN531, PN532 or PN533) */
  pn53x_type type;
  /** Chip firmware text */
  char firmware_text[22];
  /** Current power mode */
  pn53x_power_mode power_mode;
  /** Current operating mode */
  pn53x_operating_mode operating_mode;
  /** Current emulated target */
  nfc_target *current_target;
  /** Current sam mode (only applicable for PN532) */
  pn532_sam_mode sam_mode;
  /** PN53x I/O functions stored in struct */
  const struct pn53x_io *io;
  /** Last status byte returned by PN53x */
  uint8_t last_status_byte;
  /** Register cache for REG_CIU_BIT_FRAMING, SYMBOL_TX_LAST_BITS: The last TX bits setting, we need to reset this if it does not apply anymore */
  uint8_t ui8TxBits;
  /** Register cache for SetParameters function. */
  uint8_t ui8Parameters;
  /** Last sent command */
  uint8_t last_command;
  /** Interframe timer correction */
  int16_t timer_correction;
  /** Timer prescaler */
  uint16_t timer_prescaler;
  /** WriteBack cache */
  uint8_t wb_data[PN53X_CACHE_REGISTER_SIZE];
  uint8_t wb_mask[PN53X_CACHE_REGISTER_SIZE];
  bool wb_trigged;
  /** Command timeout */
  int timeout_command;
  /** ATR timeout */
  int timeout_atr;
  /** Communication timeout */
  int timeout_communication;
  /** Supported modulation type */
  nfc_modulation_type *supported_modulation_as_initiator;
  nfc_modulation_type *supported_modulation_as_target;
};

#define CHIP_DATA(pnd) ((struct pn53x_data*)(pnd->chip_data))

/**
 * @enum pn53x_modulation
 * @brief NFC modulation enumeration
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
} pn53x_modulation;

/**
 * @enum pn53x_target_type
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
} pn53x_target_type;

/**
 * @enum pn53x_target_mode
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
} pn53x_target_mode;

extern const uint8_t pn53x_ack_frame[PN53x_ACK_FRAME__LEN];
extern const uint8_t pn53x_nack_frame[PN53x_ACK_FRAME__LEN];

int    pn53x_init(struct nfc_device *pnd);
int    pn53x_transceive(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRxLen, int timeout);

int    pn53x_set_parameters(struct nfc_device *pnd, const uint8_t ui8Value, const bool bEnable);
int    pn53x_set_tx_bits(struct nfc_device *pnd, const uint8_t ui8Bits);
int    pn53x_wrap_frame(const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtFrame);
int    pn53x_unwrap_frame(const uint8_t *pbtFrame, const size_t szFrameBits, uint8_t *pbtRx, uint8_t *pbtRxPar);
int    pn53x_decode_target_data(const uint8_t *pbtRawData, size_t szRawData,
                                pn53x_type chip_type, nfc_modulation_type nmt,
                                nfc_target_info *pnti);
int    pn53x_read_register(struct nfc_device *pnd, uint16_t ui16Reg, uint8_t *ui8Value);
int    pn53x_write_register(struct nfc_device *pnd, uint16_t ui16Reg, uint8_t ui8SymbolMask, uint8_t ui8Value);
int    pn53x_decode_firmware_version(struct nfc_device *pnd);
int    pn53x_set_property_int(struct nfc_device *pnd, const nfc_property property, const int value);
int    pn53x_set_property_bool(struct nfc_device *pnd, const nfc_property property, const bool bEnable);

int    pn53x_check_communication(struct nfc_device *pnd);
int    pn53x_idle(struct nfc_device *pnd);

// NFC device as Initiator functions
int    pn53x_initiator_init(struct nfc_device *pnd);
int    pn532_initiator_init_secure_element(struct nfc_device *pnd);
int    pn53x_initiator_select_passive_target(struct nfc_device *pnd,
                                             const nfc_modulation nm,
                                             const uint8_t *pbtInitData, const size_t szInitData,
                                             nfc_target *pnt);
int    pn53x_initiator_poll_target(struct nfc_device *pnd,
                                   const nfc_modulation *pnmModulations, const size_t szModulations,
                                   const uint8_t uiPollNr, const uint8_t uiPeriod,
                                   nfc_target *pnt);
int    pn53x_initiator_select_dep_target(struct nfc_device *pnd,
                                         const nfc_dep_mode ndm, const nfc_baud_rate nbr,
                                         const nfc_dep_info *pndiInitiator,
                                         nfc_target *pnt,
                                         const int timeout);
int    pn53x_initiator_transceive_bits(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits,
                                       const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar);
int    pn53x_initiator_transceive_bytes(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx,
                                        uint8_t *pbtRx, const size_t szRx, int timeout);
int    pn53x_initiator_transceive_bits_timed(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits,
                                             const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar, uint32_t *cycles);
int    pn53x_initiator_transceive_bytes_timed(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx,
                                              uint8_t *pbtRx, const size_t szRx, uint32_t *cycles);
int    pn53x_initiator_deselect_target(struct nfc_device *pnd);
int    pn53x_initiator_target_is_present(struct nfc_device *pnd, const nfc_target *pnt);

// NFC device as Target functions
int    pn53x_target_init(struct nfc_device *pnd, nfc_target *pnt, uint8_t *pbtRx, const size_t szRxLen, int timeout);
int    pn53x_target_receive_bits(struct nfc_device *pnd, uint8_t *pbtRx, const size_t szRxLen, uint8_t *pbtRxPar);
int    pn53x_target_receive_bytes(struct nfc_device *pnd, uint8_t *pbtRx, const size_t szRxLen, int timeout);
int    pn53x_target_send_bits(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar);
int    pn53x_target_send_bytes(struct nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, int timeout);

// Error handling functions
const char *pn53x_strerror(const struct nfc_device *pnd);

// C wrappers for PN53x commands
int    pn53x_SetParameters(struct nfc_device *pnd, const uint8_t ui8Value);
int    pn532_SAMConfiguration(struct nfc_device *pnd, const pn532_sam_mode mode, int timeout);
int    pn53x_PowerDown(struct nfc_device *pnd);
int    pn53x_InListPassiveTarget(struct nfc_device *pnd, const pn53x_modulation pmInitModulation,
                                 const uint8_t szMaxTargets, const uint8_t *pbtInitiatorData,
                                 const size_t szInitiatorDataLen, uint8_t *pbtTargetsData, size_t *pszTargetsData,
                                 int timeout);
int    pn53x_InDeselect(struct nfc_device *pnd, const uint8_t ui8Target);
int    pn53x_InRelease(struct nfc_device *pnd, const uint8_t ui8Target);
int    pn53x_InAutoPoll(struct nfc_device *pnd, const pn53x_target_type *ppttTargetTypes, const size_t szTargetTypes,
                        const uint8_t btPollNr, const uint8_t btPeriod, nfc_target *pntTargets,
                        const int timeout);
int    pn53x_InJumpForDEP(struct nfc_device *pnd,
                          const nfc_dep_mode ndm, const nfc_baud_rate nbr,
                          const uint8_t *pbtPassiveInitiatorData,
                          const uint8_t *pbtNFCID3i,
                          const uint8_t *pbtGB, const size_t szGB,
                          nfc_target *pnt,
                          const int timeout);
int    pn53x_TgInitAsTarget(struct nfc_device *pnd, pn53x_target_mode ptm,
                            const uint8_t *pbtMifareParams,
                            const uint8_t *pbtTkt, size_t szTkt,
                            const uint8_t *pbtFeliCaParams,
                            const uint8_t *pbtNFCID3t, const uint8_t *pbtGB, const size_t szGB,
                            uint8_t *pbtRx, const size_t szRxLen, uint8_t *pbtModeByte, int timeout);

// RFConfiguration
int    pn53x_RFConfiguration__RF_field(struct nfc_device *pnd, bool bEnable);
int    pn53x_RFConfiguration__Various_timings(struct nfc_device *pnd, const uint8_t fATR_RES_Timeout, const uint8_t fRetryTimeout);
int    pn53x_RFConfiguration__MaxRtyCOM(struct nfc_device *pnd, const uint8_t MaxRtyCOM);
int    pn53x_RFConfiguration__MaxRetries(struct nfc_device *pnd, const uint8_t MxRtyATR, const uint8_t MxRtyPSL, const uint8_t MxRtyPassiveActivation);

// Misc
int    pn53x_check_ack_frame(struct nfc_device *pnd, const uint8_t *pbtRxFrame, const size_t szRxFrameLen);
int    pn53x_check_error_frame(struct nfc_device *pnd, const uint8_t *pbtRxFrame, const size_t szRxFrameLen);
int    pn53x_build_frame(uint8_t *pbtFrame, size_t *pszFrame, const uint8_t *pbtData, const size_t szData);
int    pn53x_get_supported_modulation(nfc_device *pnd, const nfc_mode mode, const nfc_modulation_type **const supported_mt);
int    pn53x_get_supported_baud_rate(nfc_device *pnd, const nfc_modulation_type nmt, const nfc_baud_rate **const supported_br);
int    pn53x_get_information_about(nfc_device *pnd, char **pbuf);

void   *pn53x_data_new(struct nfc_device *pnd, const struct pn53x_io *io);
void    pn53x_data_free(struct nfc_device *pnd);

#endif // __NFC_CHIPS_PN53X_H__
