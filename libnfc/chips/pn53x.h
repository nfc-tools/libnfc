/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, 2010, Roel Verdult, Romuald Conty
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
#define __NFC_CHIPS_PN53X_H__

#include <nfc/nfc-types.h>

#define MAX_FRAME_LEN       264

// Registers and symbols masks used to covers parts within a register
#define REG_CIU_TX_MODE           0x6302
  #define SYMBOL_TX_CRC_ENABLE      0x80
#define REG_CIU_RX_MODE           0x6303
  #define SYMBOL_RX_CRC_ENABLE      0x80
  #define SYMBOL_RX_NO_ERROR        0x08
  #define SYMBOL_RX_MULTIPLE        0x04
#define REG_CIU_TX_AUTO           0x6305
  #define SYMBOL_FORCE_100_ASK      0x40
  #define SYMBOL_AUTO_WAKE_UP       0x20
  #define SYMBOL_INITIAL_RF_ON      0x04
#define REG_CIU_MANUAL_RCV        0x630D
  #define SYMBOL_PARITY_DISABLE     0x10
#define REG_CIU_STATUS2           0x6338
  #define SYMBOL_MF_CRYPTO1_ON      0x08
#define REG_CIU_CONTROL           0x633C
  #define SYMBOL_INITIATOR          0x10
  #define SYMBOL_RX_LAST_BITS       0x07
#define REG_CIU_BIT_FRAMING       0x633D
  #define SYMBOL_TX_LAST_BITS       0x07

#define SYMBOL_PARAM_fAutomaticRATS     0x10
#define SYMBOL_PARAM_fAutomaticATR_RES  0x04

// Internal parameters flags
#define PARAM_NONE                  0x00
#define PARAM_NAD_USED              0x01
#define PARAM_DID_USED              0x02
#define PARAM_AUTO_ATR_RES          0x04
#define PARAM_AUTO_RATS             0x10
#define PARAM_14443_4_PICC          0x20
#define PARAM_NO_AMBLE              0x40

// Radio Field Configure Items           // Configuration Data length
#define RFCI_FIELD                  0x01 //  1 
#define RFCI_TIMING                 0x02 //  3
#define RFCI_RETRY_DATA             0x04 //  1
#define RFCI_RETRY_SELECT           0x05 //  3
#define RFCI_ANALOG_TYPE_A_106      0x0A // 11
#define RFCI_ANALOG_TYPE_A_212_424  0x0B //  8
#define RFCI_ANALOG_TYPE_B          0x0C //  3
#define RFCI_ANALOG_TYPE_14443_4    0x0D //  9

/* PN53x specific device-level errors */
#define DENACK          0x0100        /* NACK */
#define DEACKMISMATCH   0x0200        /* Unexpected data */
#define DEISERRFRAME    0x0300        /* Error frame */
#define DENOTSUP        0x0400        /* Not supported */

bool pn53x_transceive_check_ack_frame_callback(nfc_device_t* pnd, const byte_t *pbtRxFrame, const size_t szRxFrameLen);
bool pn53x_transceive_check_error_frame_callback(nfc_device_t* pnd, const byte_t *pbtRxFrame, const size_t szRxFrameLen);
bool pn53x_transceive(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);
byte_t pn53x_get_reg(nfc_device_t* pnd, uint16_t ui16Reg);
bool pn53x_set_reg(nfc_device_t* pnd, uint16_t ui16Reg, uint8_t ui8SybmolMask, uint8_t ui8Value);
bool pn53x_set_parameters(nfc_device_t* pnd, uint8_t ui8Value);
bool pn53x_set_tx_bits(nfc_device_t* pnd, uint8_t ui8Bits);
bool pn53x_wrap_frame(const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtFrame, size_t* pszFrameBits);
bool pn53x_unwrap_frame(const byte_t* pbtFrame, const size_t szFrameBits, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);
bool pn53x_decode_target_data(const byte_t* pbtRawData, size_t szDataLen, nfc_chip_t nc, nfc_target_type_t ntt, nfc_target_info_t* pnti);

bool pn53x_InListPassiveTarget(nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t szMaxTargets, const byte_t* pbtInitiatorData, const size_t szInitiatorDataLen, byte_t* pbtTargetsData, size_t* pszTargetsData);
bool pn53x_InDeselect(nfc_device_t* pnd, const uint8_t ui8Target);
bool pn53x_InRelease(nfc_device_t* pnd, const uint8_t ui8Target);
bool pn53x_InAutoPoll(nfc_device_t* pnd, const nfc_target_type_t* pnttTargetTypes, const size_t szTargetTypes, const byte_t btPollNr, const byte_t btPeriod, nfc_target_t* pntTargets, size_t* pszTargetFound);

bool pn53x_get_firmware_version (nfc_device_t *pnd);
bool pn53x_configure(nfc_device_t* pnd, const nfc_device_option_t ndo, const bool bEnable);
bool pn53x_select_dep_target(nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t* pnti);
bool pn53x_transceive_dep_bytes(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);
bool pn53x_transceive_bits(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);
bool pn53x_transceive_bytes(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);

bool pn53x_target_receive_dep_bytes(nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen);

const char *pn53x_strerror (const nfc_device_t *pnd);

static const struct chip_callbacks pn53x_callbacks_list = {
    pn53x_strerror
};

#endif // __NFC_CHIPS_PN53X_H__

