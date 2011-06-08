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
 * @file pn53x.c
 * @brief PN531, PN532 and PN533 common functions
 */

/* vim:set ts=2 sw=2 et: */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <nfc/nfc.h>

#include "pn53x.h"
#include "pn53x-internal.h"

#include "mirror-subr.h"
#include "nfc-internal.h"

#include <sys/param.h>

// TODO: Count max bytes for InJumpForDEP reply
const byte_t pncmd_initiator_jump_for_dep[68] = { 0xD4, 0x56 };

static const byte_t pn53x_ack_frame[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
static const byte_t pn53x_nack_frame[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };
static const byte_t pn53x_error_frame[] = { 0x00, 0x00, 0xff, 0x01, 0xff, 0x7f, 0x81, 0x00 };

/* prototypes */
bool pn53x_reset_settings (nfc_device_t * pnd);
bool pn53x_writeback_register (nfc_device_t * pnd);

nfc_modulation_t pn53x_ptt_to_nm (const pn53x_target_type_t ptt);
pn53x_modulation_t pn53x_nm_to_pm (const nfc_modulation_t nm);
pn53x_target_type_t pn53x_nm_to_ptt (const nfc_modulation_t nm);

bool
pn53x_init(nfc_device_t * pnd)
{
  // GetFirmwareVersion command is used to set PN53x chips type (PN531, PN532 or PN533)
  char abtFirmwareText[22];
  if (!pn53x_get_firmware_version (pnd, abtFirmwareText)) {
    return false;
  }

  // CRC handling should be enabled by default as declared in nfc_device_new
  // which is the case by default for pn53x, so nothing to do here
  // Parity handling should be enabled by default as declared in nfc_device_new
  // which is the case by default for pn53x, so nothing to do here

  // We can't read these parameters, so we set a default config by using the SetParameters wrapper
  // Note: pn53x_SetParameters() will save the sent value in pnd->ui8Parameters cache
  if(!pn53x_SetParameters(pnd, PARAM_AUTO_ATR_RES | PARAM_AUTO_RATS)) {
    return false;
  }

  pn53x_reset_settings(pnd);

  // Add the firmware revision to the device name
  char   *pcName;
  pcName = strdup (pnd->acName);
  snprintf (pnd->acName, DEVICE_NAME_LENGTH - 1, "%s - %s", pcName, abtFirmwareText);
  free (pcName);
  return true;
}

bool
pn53x_reset_settings(nfc_device_t * pnd)
{
  // Reset the ending transmission bits register, it is unknown what the last tranmission used there
  CHIP_DATA (pnd)->ui8TxBits = 0;
  if (!pn53x_write_register (pnd, PN53X_REG_CIU_BitFraming, SYMBOL_TX_LAST_BITS, 0x00)) {
    return false;
  }
  return true;
}

bool
pn53x_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx, size_t *pszRx)
{
  if (CHIP_DATA (pnd)->wb_trigged) {
    if (!pn53x_writeback_register (pnd)) {
      return false;
    }
  }

  PNCMD_DBG (pbtTx[0]);
  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof(abtRx);

  // Check if receiving buffers are available, if not, replace them
  if (!pszRx || !pbtRx) {
    pbtRx = abtRx;
    pszRx = &szRx;
  }

  // Call the send/receice callback functions of the current driver
  if (!CHIP_DATA (pnd)->io->send (pnd, pbtTx, szTx))
    return false;

  // Command is sent, we store the command
  CHIP_DATA (pnd)->ui8LastCommand = pbtTx[0];

  // Handle power mode for PN532
  if ((CHIP_DATA (pnd)->type == PN532) && (TgInitAsTarget == pbtTx[0])) { // PN532 automatically goes into PowerDown mode when TgInitAsTarget command will be sent
    CHIP_DATA (pnd)->power_mode = POWERDOWN;
  }

  int res = CHIP_DATA(pnd)->io->receive (pnd, pbtRx, *pszRx);
  if (res < 0) {
    return false;
  }

  if (pnd->iLastError)
    return false;

  if ((CHIP_DATA(pnd)->type == PN532) && (TgInitAsTarget == pbtTx[0])) { // PN532 automatically wakeup on external RF field
    CHIP_DATA(pnd)->power_mode = NORMAL; // When TgInitAsTarget reply that means an external RF have waken up the chip
  }

  *pszRx = (size_t) res;
  switch (pbtTx[0]) {
    case PowerDown:
    case InDataExchange:
    case InCommunicateThru:
    case InJumpForPSL:
    case InPSL:
    case InATR:
    case InSelect:
    case InJumpForDEP:
    case TgGetData:
    case TgGetInitiatorCommand:
    case TgSetData:
    case TgResponseToInitiator:
    case TgSetGeneralBytes:
    case TgSetMetaData:
      pnd->iLastError = pbtRx[0] & 0x3f;
      break;
    case InDeselect:
    case InRelease:
      if (CHIP_DATA(pnd)->type == RCS360) {
        // Error code is in pbtRx[1] but we ignore error code anyway
        // because other PN53x chips always return 0 on those commands
        pnd->iLastError = 0;
        break;
      }
      pnd->iLastError = pbtRx[0] & 0x3f;
      break;
    default:
      pnd->iLastError = 0;
  }
  if (CHIP_DATA(pnd)->type == PN533) {
    if ((pbtTx[0] == ReadRegister) || (pbtTx[0] == WriteRegister)) {
      // PN533 prepends its answer by a status byte
      pnd->iLastError = pbtRx[0] & 0x3f;
    }
  }
  return (0 == pnd->iLastError);
}

bool
pn53x_set_parameters (nfc_device_t * pnd, const uint8_t ui8Parameter, const bool bEnable)
{
  uint8_t ui8Value = (bEnable) ? (CHIP_DATA (pnd)->ui8Parameters | ui8Parameter) : (CHIP_DATA (pnd)->ui8Parameters & ~(ui8Parameter));
  if (ui8Value != CHIP_DATA (pnd)->ui8Parameters) {
    return pn53x_SetParameters(pnd, ui8Value);
  }
  return true;
}

bool
pn53x_set_tx_bits (nfc_device_t * pnd, const uint8_t ui8Bits)
{
  // Test if we need to update the transmission bits register setting
  if (CHIP_DATA (pnd)->ui8TxBits != ui8Bits) {
    // Set the amount of transmission bits in the PN53X chip register
    if (!pn53x_write_register (pnd, PN53X_REG_CIU_BitFraming, SYMBOL_TX_LAST_BITS, ui8Bits))
      return false;

    // Store the new setting
    CHIP_DATA (pnd)->ui8TxBits = ui8Bits;
  }
  return true;
}

bool
pn53x_wrap_frame (const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar,
                  byte_t * pbtFrame, size_t * pszFrameBits)
{
  byte_t  btFrame;
  byte_t  btData;
  uint32_t uiBitPos;
  uint32_t uiDataPos = 0;
  size_t  szBitsLeft = szTxBits;

  // Make sure we should frame at least something
  if (szBitsLeft == 0)
    return false;

  // Handle a short response (1byte) as a special case
  if (szBitsLeft < 9) {
    *pbtFrame = *pbtTx;
    *pszFrameBits = szTxBits;
    return true;
  }
  // We start by calculating the frame length in bits
  *pszFrameBits = szTxBits + (szTxBits / 8);

  // Parse the data bytes and add the parity bits
  // This is really a sensitive process, mirror the frame bytes and append parity bits
  // buffer = mirror(frame-byte) + parity + mirror(frame-byte) + parity + ...
  // split "buffer" up in segments of 8 bits again and mirror them
  // air-bytes = mirror(buffer-byte) + mirror(buffer-byte) + mirror(buffer-byte) + ..
  while (true) {
    // Reset the temporary frame byte;
    btFrame = 0;

    for (uiBitPos = 0; uiBitPos < 8; uiBitPos++) {
      // Copy as much data that fits in the frame byte
      btData = mirror (pbtTx[uiDataPos]);
      btFrame |= (btData >> uiBitPos);
      // Save this frame byte
      *pbtFrame = mirror (btFrame);
      // Set the remaining bits of the date in the new frame byte and append the parity bit
      btFrame = (btData << (8 - uiBitPos));
      btFrame |= ((pbtTxPar[uiDataPos] & 0x01) << (7 - uiBitPos));
      // Backup the frame bits we have so far
      pbtFrame++;
      *pbtFrame = mirror (btFrame);
      // Increase the data (without parity bit) position
      uiDataPos++;
      // Test if we are done
      if (szBitsLeft < 9)
        return true;
      szBitsLeft -= 8;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFrame++;
  }
}

bool
pn53x_unwrap_frame (const byte_t * pbtFrame, const size_t szFrameBits, byte_t * pbtRx, size_t * pszRxBits,
                    byte_t * pbtRxPar)
{
  byte_t  btFrame;
  byte_t  btData;
  uint8_t uiBitPos;
  uint32_t uiDataPos = 0;
  byte_t *pbtFramePos = (byte_t *) pbtFrame;
  size_t  szBitsLeft = szFrameBits;

  // Make sure we should frame at least something
  if (szBitsLeft == 0)
    return false;

  // Handle a short response (1byte) as a special case
  if (szBitsLeft < 9) {
    *pbtRx = *pbtFrame;
    *pszRxBits = szFrameBits;
    return true;
  }
  // Calculate the data length in bits
  *pszRxBits = szFrameBits - (szFrameBits / 9);

  // Parse the frame bytes, remove the parity bits and store them in the parity array
  // This process is the reverse of WrapFrame(), look there for more info
  while (true) {
    for (uiBitPos = 0; uiBitPos < 8; uiBitPos++) {
      btFrame = mirror (pbtFramePos[uiDataPos]);
      btData = (btFrame << uiBitPos);
      btFrame = mirror (pbtFramePos[uiDataPos + 1]);
      btData |= (btFrame >> (8 - uiBitPos));
      pbtRx[uiDataPos] = mirror (btData);
      if (pbtRxPar != NULL)
        pbtRxPar[uiDataPos] = ((btFrame >> (7 - uiBitPos)) & 0x01);
      // Increase the data (without parity bit) position
      uiDataPos++;
      // Test if we are done
      if (szBitsLeft < 9)
        return true;
      szBitsLeft -= 9;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFramePos++;
  }
}

bool
pn53x_decode_target_data (const byte_t * pbtRawData, size_t szRawData, pn53x_type type, nfc_modulation_type_t nmt,
                          nfc_target_info_t * pnti)
{
  uint8_t szAttribRes;

  switch (nmt) {
    case NMT_ISO14443A:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;

      // Somehow they switched the lower and upper ATQA bytes around for the PN531 chipset
      if (type == PN531) {
        pnti->nai.abtAtqa[1] = *(pbtRawData++);
        pnti->nai.abtAtqa[0] = *(pbtRawData++);
      } else {
        pnti->nai.abtAtqa[0] = *(pbtRawData++);
        pnti->nai.abtAtqa[1] = *(pbtRawData++);
      }
      pnti->nai.btSak = *(pbtRawData++);
      // Copy the NFCID1
      pnti->nai.szUidLen = *(pbtRawData++);
      memcpy (pnti->nai.abtUid, pbtRawData, pnti->nai.szUidLen);
      pbtRawData += pnti->nai.szUidLen;

      // Did we received an optional ATS (Smardcard ATR)
      if (szRawData > (pnti->nai.szUidLen + 5)) {
        pnti->nai.szAtsLen = ((*(pbtRawData++)) - 1);     // In pbtRawData, ATS Length byte is counted in ATS Frame.
        memcpy (pnti->nai.abtAts, pbtRawData, pnti->nai.szAtsLen);
      } else {
        pnti->nai.szAtsLen = 0;
      }

      // Strip CT (Cascade Tag) to retrieve and store the _real_ UID
      // (e.g. 0x8801020304050607 is in fact 0x01020304050607)
      if ((pnti->nai.szUidLen == 8) && (pnti->nai.abtUid[0] == 0x88)) {
        pnti->nai.szUidLen = 7;
        memmove (pnti->nai.abtUid, pnti->nai.abtUid + 1, 7);
      } else if ((pnti->nai.szUidLen == 12) && (pnti->nai.abtUid[0] == 0x88) && (pnti->nai.abtUid[4] == 0x88)) {
        pnti->nai.szUidLen = 10;
        memmove (pnti->nai.abtUid, pnti->nai.abtUid + 1, 3);
        memmove (pnti->nai.abtUid + 3, pnti->nai.abtUid + 5, 7);
      }
      break;

    case NMT_ISO14443B:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;

      // Now we are in ATQB, we skip the first ATQB byte always equal to 0x50
      pbtRawData++;

      // Store the PUPI (Pseudo-Unique PICC Identifier)
      memcpy (pnti->nbi.abtPupi, pbtRawData, 4);
      pbtRawData += 4;

      // Store the Application Data
      memcpy (pnti->nbi.abtApplicationData, pbtRawData, 4);
      pbtRawData += 4;

      // Store the Protocol Info
      memcpy (pnti->nbi.abtProtocolInfo, pbtRawData, 3);
      pbtRawData += 3;

      // We leave the ATQB field, we now enter in Card IDentifier
      szAttribRes = *(pbtRawData++);
      if (szAttribRes) {
        pnti->nbi.ui8CardIdentifier = *(pbtRawData++);
      }
      break;

    case NMT_ISO14443BI:
      // Skip V & T Addresses
      pbtRawData++;
      if (*pbtRawData != 0x07) { // 0x07 = REPGEN 
        return false;
      }
      pbtRawData++;
      // Store the UID
      memcpy (pnti->nii.abtDIV, pbtRawData, 4);
      pbtRawData += 4;
      pnti->nii.btVerLog = *(pbtRawData++);
      if (pnti->nii.btVerLog & 0x80) { // Type = long?
        pnti->nii.btConfig = *(pbtRawData++);
        if (pnti->nii.btConfig & 0x40) {
          memcpy (pnti->nii.abtAtr, pbtRawData, szRawData - 8);
          pbtRawData += szRawData - 8;
          pnti->nii.szAtrLen = szRawData - 8;
        }
      }
      break;

    case NMT_ISO14443B2SR:
      // Store the UID
      memcpy (pnti->nsi.abtUID, pbtRawData, 8);
      pbtRawData += 8;
      break;

    case NMT_ISO14443B2CT:
      // Store UID LSB
      memcpy (pnti->nci.abtUID, pbtRawData, 2);
      pbtRawData += 2;
      // Store Prod Code & Fab Code
      pnti->nci.btProdCode = *(pbtRawData++);
      pnti->nci.btFabCode = *(pbtRawData++);
      // Store UID MSB
      memcpy (pnti->nci.abtUID+2, pbtRawData, 2);
      pbtRawData += 2;
      break;

    case NMT_FELICA:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;

      // Store the mandatory info
      pnti->nfi.szLen = *(pbtRawData++);
      pnti->nfi.btResCode = *(pbtRawData++);
      // Copy the NFCID2t
      memcpy (pnti->nfi.abtId, pbtRawData, 8);
      pbtRawData += 8;
      // Copy the felica padding
      memcpy (pnti->nfi.abtPad, pbtRawData, 8);
      pbtRawData += 8;
      // Test if the System code (SYST_CODE) is available
      if (pnti->nfi.szLen > 18) {
        memcpy (pnti->nfi.abtSysCode, pbtRawData, 2);
      }
      break;
    case NMT_JEWEL:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;

      // Store the mandatory info
      memcpy (pnti->nji.btSensRes, pbtRawData, 2);
      pbtRawData += 2;
      memcpy (pnti->nji.btId, pbtRawData, 4);
      break;
    default:
      return false;
      break;
  }
  return true;
}

bool
pn53x_ReadRegister (nfc_device_t * pnd, uint16_t ui16RegisterAddress, uint8_t * ui8Value)
{
  byte_t  abtCmd[] = { ReadRegister, ui16RegisterAddress >> 8, ui16RegisterAddress & 0xff };
  byte_t  abtRegValue[2];
  size_t  szRegValue = sizeof (abtRegValue);

  DBG ("ReadRegister (%04x)", ui16RegisterAddress);
  if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), abtRegValue, &szRegValue)) {
    return false;
  }
  if (CHIP_DATA(pnd)->type == PN533) {
    // PN533 prepends its answer by a status byte
    *ui8Value = abtRegValue[1];
  } else {
    *ui8Value = abtRegValue[0];
  }
  return true;
}

bool pn53x_read_register (nfc_device_t * pnd, uint16_t ui16RegisterAddress, uint8_t * ui8Value)
{
  return pn53x_ReadRegister (pnd, ui16RegisterAddress, ui8Value);
}

bool
pn53x_WriteRegister (nfc_device_t * pnd, const uint16_t ui16RegisterAddress, const uint8_t ui8Value)
{
  byte_t  abtCmd[] = { WriteRegister, ui16RegisterAddress >> 8, ui16RegisterAddress & 0xff, ui8Value };
  PNREG_DBG (ui16RegisterAddress);
  DBG ("WriteRegister (%04x, %02x)", ui16RegisterAddress, ui8Value);
  return pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL);
}

bool
pn53x_write_register (nfc_device_t * pnd, const uint16_t ui16RegisterAddress, const uint8_t ui8SymbolMask, const uint8_t ui8Value)
{
  if ((ui16RegisterAddress < PN53X_CACHE_REGISTER_MIN_ADDRESS) || (ui16RegisterAddress > PN53X_CACHE_REGISTER_MAX_ADDRESS)) {
    // Direct write
    if (ui8SymbolMask != 0xff) {
      uint8_t ui8CurrentValue;
      if (!pn53x_read_register (pnd, ui16RegisterAddress, &ui8CurrentValue))
        return false;
      uint8_t ui8NewValue = ((ui8Value & ui8SymbolMask) | (ui8CurrentValue & (~ui8SymbolMask)));
      if (ui8NewValue != ui8CurrentValue) {
        return pn53x_WriteRegister (pnd, ui16RegisterAddress, ui8NewValue);
      }
    } else {
      return pn53x_WriteRegister (pnd, ui16RegisterAddress, ui8Value);
    }
  } else {
    // Write-back cache area
    const int internal_address = ui16RegisterAddress - PN53X_CACHE_REGISTER_MIN_ADDRESS;
    CHIP_DATA (pnd)->wb_data[internal_address] = (CHIP_DATA (pnd)->wb_data[internal_address] & CHIP_DATA (pnd)->wb_mask[internal_address] & (~ui8SymbolMask)) | (ui8Value & ui8SymbolMask);
    CHIP_DATA (pnd)->wb_mask[internal_address] = CHIP_DATA (pnd)->wb_mask[internal_address] | ui8SymbolMask;
    CHIP_DATA (pnd)->wb_trigged = true;
    DBG ("WriteBackRegister (%04x, %02x, %02x)", ui16RegisterAddress, CHIP_DATA (pnd)->wb_data[internal_address], CHIP_DATA (pnd)->wb_mask[internal_address]);
  }
  return true;
}

bool
pn53x_writeback_register (nfc_device_t * pnd)
{
  // TODO Check at each step (ReadRegister, WriteRegister) if we didn't exceed max supported frame length
  BUFFER_INIT (abtReadRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
  BUFFER_APPEND (abtReadRegisterCmd, ReadRegister);

  // First step, it looks for registers to be read before applying the requested mask
  CHIP_DATA (pnd)->wb_trigged = false;
  for (size_t n = 0; n < PN53X_CACHE_REGISTER_SIZE; n++) {
    if ((CHIP_DATA (pnd)->wb_mask[n]) && (CHIP_DATA (pnd)->wb_mask[n] != 0xff)) {
      // This register needs to be read: mask is present but does not cover full data width (ie. mask != 0xff)
      const uint16_t pn53x_register_address = PN53X_CACHE_REGISTER_MIN_ADDRESS + n;
      BUFFER_APPEND (abtReadRegisterCmd, pn53x_register_address  >> 8);
      BUFFER_APPEND (abtReadRegisterCmd, pn53x_register_address & 0xff);
    }
  }

  if (BUFFER_SIZE (abtReadRegisterCmd) > 1) {
    // It needs to read some registers
    uint8_t abtRes[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
    size_t szRes = sizeof(abtRes);
    // It transceives the previously constructed ReadRegister command
    if (!pn53x_transceive (pnd, abtReadRegisterCmd, BUFFER_SIZE (abtReadRegisterCmd), abtRes, &szRes)) {
      return false;
    }
    size_t i = 0;
    if (CHIP_DATA(pnd)->type == PN533) {
      // PN533 prepends its answer by a status byte
      i = 1;
    }
    for (size_t n = 0; n < PN53X_CACHE_REGISTER_SIZE; n++) {
      if ((CHIP_DATA (pnd)->wb_mask[n]) && (CHIP_DATA (pnd)->wb_mask[n] != 0xff)) {
        CHIP_DATA (pnd)->wb_data[n] = ((CHIP_DATA (pnd)->wb_data[n] & CHIP_DATA (pnd)->wb_mask[n]) | (abtRes[i] & (~CHIP_DATA (pnd)->wb_mask[n])));
        if (CHIP_DATA (pnd)->wb_data[n] != abtRes[i]) {
          // Requested value is different from read one
          CHIP_DATA (pnd)->wb_mask[n] = 0xff; // We can now apply whole data bits
        } else {
          CHIP_DATA (pnd)->wb_mask[n] = 0x00; // We already have the right value
        }
        i++;
      }
    }
  }
  // Now, the writeback-cache only has masks with 0xff, we can start to WriteRegister
  BUFFER_INIT (abtWriteRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
  BUFFER_APPEND (abtWriteRegisterCmd, WriteRegister);
  for (size_t n = 0; n < PN53X_CACHE_REGISTER_SIZE; n++) {
    if (CHIP_DATA (pnd)->wb_mask[n] == 0xff) {
      const uint16_t pn53x_register_address = PN53X_CACHE_REGISTER_MIN_ADDRESS + n;
      BUFFER_APPEND (abtWriteRegisterCmd, pn53x_register_address  >> 8);
      BUFFER_APPEND (abtWriteRegisterCmd, pn53x_register_address & 0xff);
      BUFFER_APPEND (abtWriteRegisterCmd, CHIP_DATA (pnd)->wb_data[n]);
      DBG ("WriteBackRegister will write (%04x, %02x)", pn53x_register_address, CHIP_DATA (pnd)->wb_data[n]);
      // This register is handled, we reset the mask to prevent
      CHIP_DATA (pnd)->wb_mask[n] = 0x00;
    }
  }

  if (BUFFER_SIZE (abtWriteRegisterCmd) > 1) {
    // We need to write some registers
    if (!pn53x_transceive (pnd, abtWriteRegisterCmd, BUFFER_SIZE (abtWriteRegisterCmd), NULL, NULL)) {
      return false;
    }
  }
  return true;
}

bool
pn53x_get_firmware_version (nfc_device_t * pnd, char abtFirmwareText[22])
{
  const byte_t abtCmd[] = { GetFirmwareVersion };
  byte_t  abtFw[4];
  size_t  szFwLen = sizeof (abtFw);
  if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), abtFw, &szFwLen)) {
    return false;
  }
  // Determine which version of chip it is: PN531 will return only 2 bytes, while others return 4 bytes and have the first to tell the version IC
  if (szFwLen == 2) {
    CHIP_DATA(pnd)->type = PN531;
  } else if (szFwLen == 4) {
    if (abtFw[0] == 0x32) { // PN532 version IC
      CHIP_DATA(pnd)->type = PN532;
    } else if (abtFw[0] == 0x33)  { // PN533 version IC
      if (abtFw[1] == 0x01) { // Sony ROM code
        CHIP_DATA(pnd)->type = RCS360;
      } else {
        CHIP_DATA(pnd)->type = PN533;
      }
    } else {
      // Unknown version IC
      return false;
    }
  } else {
    // Unknown chip
    return false;
  }
  // Convert firmware info in text, PN531 gives 2 bytes info, but PN532 and PN533 gives 4
  switch (CHIP_DATA(pnd)->type) {
    case PN531:
      snprintf (abtFirmwareText, 22, "PN531 v%d.%d", abtFw[0], abtFw[1]);
      pnd->btSupportByte = SUPPORT_ISO14443A | SUPPORT_ISO18092;
      break;
    case PN532:
      snprintf (abtFirmwareText, 22, "PN532 v%d.%d (0x%02x)", abtFw[1], abtFw[2], abtFw[3]);
      pnd->btSupportByte = abtFw[3];
      break;
    case PN533:
    case RCS360:
      snprintf (abtFirmwareText, 22, "PN533 v%d.%d (0x%02x)", abtFw[1], abtFw[2], abtFw[3]);
      pnd->btSupportByte = abtFw[3];
      break;
  }
  return true;
}

bool
pn53x_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable)
{
  byte_t  btValue;
  switch (ndo) {
    case NDO_HANDLE_CRC:
      // Enable or disable automatic receiving/sending of CRC bytes
      if (bEnable == pnd->bCrc) {
        // Nothing to do
        return true;
      }
      // TX and RX are both represented by the symbol 0x80
      btValue = (bEnable) ? 0x80 : 0x00;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxMode, SYMBOL_TX_CRC_ENABLE, btValue))
        return false;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_CRC_ENABLE, btValue))
        return false;
      pnd->bCrc = bEnable;
      break;

    case NDO_HANDLE_PARITY:
      // Handle parity bit by PN53X chip or parse it as data bit
      if (bEnable == pnd->bPar)
        // Nothing to do
        return true;
      btValue = (bEnable) ? 0x00 : SYMBOL_PARITY_DISABLE;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_ManualRCV, SYMBOL_PARITY_DISABLE, btValue))
        return false;
      pnd->bPar = bEnable;
      break;

    case NDO_EASY_FRAMING:
      pnd->bEasyFraming = bEnable;
      break;

    case NDO_ACTIVATE_FIELD:
    {
      byte_t  abtCmd[] = { RFConfiguration, RFCI_FIELD, (bEnable) ? 0x01 : 0x00 };
      if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL))
        return false;
    }
    break;

    case NDO_ACTIVATE_CRYPTO1:
      btValue = (bEnable) ? SYMBOL_MF_CRYPTO1_ON : 0x00;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_Status2, SYMBOL_MF_CRYPTO1_ON, btValue))
        return false;
      break;

    case NDO_INFINITE_SELECT:
    {
      // TODO Made some research around this point:
      // timings could be tweak better than this, and maybe we can tweak timings
      // to "gain" a sort-of hardware polling (ie. like PN532 does)
      // Retry format: 0x00 means only 1 try, 0xff means infinite
      byte_t  abtCmd[] = {
        RFConfiguration,
        RFCI_RETRY_SELECT,
        (bEnable) ? 0xff : 0x00,        // MxRtyATR, default: active = 0xff, passive = 0x02
        (bEnable) ? 0xff : 0x00,        // MxRtyPSL, default: 0x01
        (bEnable) ? 0xff : 0x02         // MxRtyPassiveActivation, default: 0xff (0x00 leads to problems with PN531)
      };
      if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL))
        return false;
    }
    break;

    case NDO_ACCEPT_INVALID_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_NO_ERROR : 0x00;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_NO_ERROR, btValue))
        return false;
      break;

    case NDO_ACCEPT_MULTIPLE_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_MULTIPLE : 0x00;
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_MULTIPLE, btValue))
        return false;
      return true;
      break;

    case NDO_AUTO_ISO14443_4:
      if (bEnable == pnd->bAutoIso14443_4)
        // Nothing to do
        return true;
      pnd->bAutoIso14443_4 = bEnable;
      return pn53x_set_parameters (pnd, PARAM_AUTO_RATS, bEnable);
      break;

    case NDO_FORCE_ISO14443_A:
      if(!bEnable) {
        // Nothing to do
        return true;
      }
      // Force pn53x to be in ISO14443-A mode
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxMode, SYMBOL_TX_FRAMING, 0x00)) {
        return false;
      }
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_FRAMING, 0x00)) {
        return false;
      }
      // Set the PN53X to force 100% ASK Modified miller decoding (default for 14443A cards)
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxAuto, SYMBOL_FORCE_100_ASK, 0x40))
        return false;

      return true;
      break;

    case NDO_FORCE_ISO14443_B:
      if(!bEnable) {
        // Nothing to do
        return true;
      }
      // Force pn53x to be in ISO14443-B mode
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxMode, SYMBOL_TX_FRAMING, 0x03)) {
        return false;
      }
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_FRAMING, 0x03)) {
        return false;
      }

      return true;
      break;

    case NDO_FORCE_SPEED_106:
      if(!bEnable) {
        // Nothing to do
        return true;
      }
      // Force pn53x to be at 106 kbps
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxMode, SYMBOL_TX_SPEED, 0x00)) {
        return false;
      }
      if (!pn53x_write_register (pnd, PN53X_REG_CIU_RxMode, SYMBOL_RX_SPEED, 0x00)) {
        return false;
      }

      return true;
      break;
  }

  // When we reach this, the configuration is completed and successful
  return true;
}

bool
pn53x_idle (nfc_device_t *pnd)
{
  switch (CHIP_DATA (pnd)->operating_mode) {
    case TARGET:
      // InRelease used in target mode stops the target emulation and no more
      // tag are seen from external initiator
      if (!pn53x_InRelease (pnd, 0)) {
        return false;
      }
      if (CHIP_DATA (pnd)->type == PN532) {
        // Use PowerDown to go in "Low VBat" power mode
        if (!pn53x_PowerDown (pnd)) {
          return false;
        }
        CHIP_DATA (pnd)->power_mode = LOWVBAT;
      }
    break;
    case INITIATOR:
      // Deselect all active communications
      if (!pn53x_InDeselect (pnd, 0)) {
        return false;
      }
      // Disable RF field to avoid heating
      if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, false)) {
        return false;
      }
      if (CHIP_DATA (pnd)->type == PN532) {
        // Use PowerDown to go in "Low VBat" power mode
        if (!pn53x_PowerDown (pnd)) {
          return false;
        }
        CHIP_DATA (pnd)->power_mode = LOWVBAT;
      } else {
        // Use InRelease to go in "Standby mode"
        if (!pn53x_InRelease (pnd, 0)) {
          return false;
        }
      }
    break;
    default:
      // Nothing to do
    break;
  };
  CHIP_DATA (pnd)->operating_mode = IDLE;
  return true;
}

bool
pn53x_check_communication (nfc_device_t *pnd)
{
  const byte_t abtCmd[] = { Diagnose, 0x00, 'l', 'i', 'b', 'n', 'f', 'c' };
  const byte_t abtExpectedRx[] = { 0x00, 'l', 'i', 'b', 'n', 'f', 'c' };
  byte_t abtRx[sizeof(abtExpectedRx)];
  size_t szRx = sizeof (abtRx);

  if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), abtRx, &szRx))
    return false;

  return ((sizeof(abtExpectedRx) == szRx) && (0 == memcmp (abtRx, abtExpectedRx, sizeof(abtExpectedRx))));
}

bool
pn53x_initiator_init (nfc_device_t * pnd)
{
  pn53x_reset_settings(pnd);

  // Configure the PN53X to be an Initiator or Reader/Writer
  if (!pn53x_write_register (pnd, PN53X_REG_CIU_Control, SYMBOL_INITIATOR, 0x10))
    return false;

  CHIP_DATA (pnd)->operating_mode = INITIATOR;
  return true;
}

bool
pn53x_initiator_select_passive_target (nfc_device_t * pnd,
                                       const nfc_modulation_t nm,
                                       const byte_t * pbtInitData, const size_t szInitData,
                                       nfc_target_t * pnt)
{
  byte_t  abtTargetsData[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szTargetsData = sizeof(abtTargetsData);

  if (nm.nmt == NMT_ISO14443BI || nm.nmt == NMT_ISO14443B2SR || nm.nmt == NMT_ISO14443B2CT) {
    if (CHIP_DATA(pnd)->type == RCS360) {
      // TODO add support for RC-S360, at the moment it refuses to send raw frames without a first select
      return false;
    }
    // No native support in InListPassiveTarget so we do discovery by hand
    if (!nfc_configure (pnd, NDO_FORCE_ISO14443_B, true)) {
      return false;
    }
    if (!nfc_configure (pnd, NDO_FORCE_SPEED_106, true)) {
      return false;
    }
    if (!nfc_configure (pnd, NDO_HANDLE_CRC, true)) {
      return false;
    }
    pnd->bEasyFraming = false;
    if (nm.nmt == NMT_ISO14443B2SR) {
      // Some work to do before getting the UID...
      byte_t abtInitiate[]="\x06\x00";
      size_t szInitiateLen = 2;
      byte_t abtSelect[]="\x0e\x00";
      size_t szSelectLen = 2;
      byte_t abtRx[1];
      size_t szRxLen = 1;
      // Getting random Chip_ID
      if (!pn53x_initiator_transceive_bytes (pnd, abtInitiate, szInitiateLen, abtRx, &szRxLen)) {
        return false;
      }
      abtSelect[1] = abtRx[0];
      if (!pn53x_initiator_transceive_bytes (pnd, abtSelect, szSelectLen, abtRx, &szRxLen)) {
        return false;
      }
    }
    else if (nm.nmt == NMT_ISO14443B2CT) {
      // Some work to do before getting the UID...
      byte_t abtReqt[]="\x10";
      size_t szReqtLen = 1;
      // Getting product code / fab code & store it in output buffer after the serial nr we'll obtain later
      if (!pn53x_initiator_transceive_bytes (pnd, abtReqt, szReqtLen, abtTargetsData+2, &szTargetsData) || szTargetsData != 2) {
        return false;
      }
    }
    if (!pn53x_initiator_transceive_bytes (pnd, pbtInitData, szInitData, abtTargetsData, &szTargetsData)) {
      return false;
    }
    if (nm.nmt == NMT_ISO14443B2CT) {
      if (szTargetsData != 2)
        return false;
      byte_t abtRead[]="\xC4"; // Reading UID_MSB (Read address 4)
      size_t szReadLen = 1;
      if (!pn53x_initiator_transceive_bytes (pnd, abtRead, szReadLen, abtTargetsData+4, &szTargetsData) || szTargetsData != 2) {
        return false;
      }
      szTargetsData = 6; // u16 UID_LSB, u8 prod code, u8 fab code, u16 UID_MSB
    }
    if (pnt) {
      pnt->nm = nm;
      // Fill the tag info struct with the values corresponding to this init modulation
      if (!pn53x_decode_target_data (abtTargetsData, szTargetsData, CHIP_DATA(pnd)->type, nm.nmt, &(pnt->nti))) {
        return false;
      }
    }
    if (nm.nmt == NMT_ISO14443BI) {
      // Select tag
      byte_t abtAttrib[6];
      size_t szAttribLen = sizeof(abtAttrib);
      memcpy(abtAttrib, abtTargetsData, szAttribLen);
      abtAttrib[1] = 0x0f; // ATTRIB
      if (!pn53x_initiator_transceive_bytes (pnd, abtAttrib, szAttribLen, NULL, NULL)) {
        return false;
      }
    }
    return true;
  } // else:

  const pn53x_modulation_t pm = pn53x_nm_to_pm(nm);
  if (PM_UNDEFINED == pm) {
    pnd->iLastError = DENOTSUP;
    return false;
  }

  if (!pn53x_InListPassiveTarget (pnd, pm, 1, pbtInitData, szInitData, abtTargetsData, &szTargetsData))
    return false;

  // Make sure one tag has been found, the PN53X returns 0x00 if none was available
  if (abtTargetsData[0] == 0)
    return false;

  // Is a tag info struct available
  if (pnt) {
    pnt->nm = nm;
    // Fill the tag info struct with the values corresponding to this init modulation
    if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, CHIP_DATA(pnd)->type, nm.nmt, &(pnt->nti))) {
      return false;
    }
  }
  return true;
}

bool
pn53x_initiator_poll_targets (nfc_device_t * pnd,
                              const nfc_modulation_t * pnmModulations, const size_t szModulations,
                              const byte_t btPollNr, const byte_t btPeriod,
                              nfc_target_t * pntTargets, size_t * pszTargetFound)
{
  size_t szTargetTypes = 0;
  pn53x_target_type_t apttTargetTypes[32];
  for (size_t n=0; n<szModulations; n++) {
    const pn53x_target_type_t ptt = pn53x_nm_to_ptt(pnmModulations[n]);
    if (PTT_UNDEFINED == ptt) {
      pnd->iLastError = DENOTSUP;
      return false;
    }
    apttTargetTypes[szTargetTypes] = ptt;
    if ((pnd->bAutoIso14443_4) && (ptt == PTT_MIFARE)) { // Hack to have ATS
      apttTargetTypes[szTargetTypes] = PTT_ISO14443_4A_106;
      szTargetTypes++;
      apttTargetTypes[szTargetTypes] = PTT_MIFARE;
    }
    szTargetTypes++;
  }

  return pn53x_InAutoPoll (pnd, apttTargetTypes, szTargetTypes, btPollNr, btPeriod, pntTargets, pszTargetFound);
}

bool
pn53x_initiator_select_dep_target(nfc_device_t * pnd,
                                  const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr,
                                  const nfc_dep_info_t * pndiInitiator,
                                  nfc_target_t * pnt)
{
  const byte_t abtPassiveInitiatorData[] = { 0x00, 0xff, 0xff, 0x00, 0x00 }; // Only for 212/424 kpbs: First 4 bytes shall be set like this according to NFCIP-1, last byte is TSN (Time Slot Number)
  const byte_t * pbtPassiveInitiatorData = NULL;

  switch (nbr) {
    case NBR_212:
    case NBR_424:
      // Only use this predefined bytes array when we are at 212/424kbps
      pbtPassiveInitiatorData = abtPassiveInitiatorData;
      break;

    default:
      // Nothing to do
      break;
  }

  if (pndiInitiator) {
    return pn53x_InJumpForDEP (pnd, ndm, nbr, pbtPassiveInitiatorData, pndiInitiator->abtNFCID3, pndiInitiator->abtGB, pndiInitiator->szGB, pnt);
  } else {
    return pn53x_InJumpForDEP (pnd, ndm, nbr, pbtPassiveInitiatorData, NULL, NULL, 0, pnt);
  }
}

bool
pn53x_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                 const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  size_t  szFrameBits = 0;
  size_t  szFrameBytes = 0;
  uint8_t ui8rcc;
  uint8_t ui8Bits = 0;
  byte_t  abtCmd[PN53x_EXTENDED_FRAME__DATA_MAX_LEN] = { InCommunicateThru };

  // Check if we should prepare the parity bits ourself
  if (!pnd->bPar) {
    // Convert data with parity to a frame
    pn53x_wrap_frame (pbtTx, szTxBits, pbtTxPar, abtCmd + 1, &szFrameBits);
  } else {
    szFrameBits = szTxBits;
  }

  // Retrieve the leading bits
  ui8Bits = szFrameBits % 8;

  // Get the amount of frame bytes + optional (1 byte if there are leading bits)
  szFrameBytes = (szFrameBits / 8) + ((ui8Bits == 0) ? 0 : 1);

  // When the parity is handled before us, we just copy the data
  if (pnd->bPar)
    memcpy (abtCmd + 1, pbtTx, szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits (pnd, ui8Bits))
    return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the command byte 0x42)
  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof(abtRx);
  if (!pn53x_transceive (pnd, abtCmd, szFrameBytes + 1, abtRx, &szRx))
    return false;

  // Get the last bit-count that is stored in the received byte
  if (!pn53x_read_register (pnd, PN53X_REG_CIU_Control, &ui8rcc))
    return false;
  ui8Bits = ui8rcc & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  szFrameBits = ((szRx - 1 - ((ui8Bits == 0) ? 0 : 1)) * 8) + ui8Bits;

  if (pbtRx != NULL) {
    // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
    // Check if we should recover the parity bits ourself
    if (!pnd->bPar) {
      // Unwrap the response frame
      pn53x_unwrap_frame (abtRx + 1, szFrameBits, pbtRx, pszRxBits, pbtRxPar);
    } else {
      // Save the received bits
      *pszRxBits = szFrameBits;
      // Copy the received bytes
      memcpy (pbtRx, abtRx + 1, szRx - 1);
    }
  }
  // Everything went successful
  return true;
}

bool
pn53x_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                                  size_t * pszRx)
{
  size_t  szExtraTxLen;
  byte_t  abtCmd[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) {
    pnd->iLastError = DENOTSUP;
    return false;
  }

  // Copy the data into the command frame
  if (pnd->bEasyFraming) {
    abtCmd[0] = InDataExchange;
    abtCmd[1] = 1;              /* target number */
    memcpy (abtCmd + 2, pbtTx, szTx);
    szExtraTxLen = 2;
  } else {
    abtCmd[0] = InCommunicateThru;
    memcpy (abtCmd + 1, pbtTx, szTx);
    szExtraTxLen = 1;
  }

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits (pnd, 0))
    return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof(abtRx);
  if (!pn53x_transceive (pnd, abtCmd, szTx + szExtraTxLen, abtRx, &szRx))
    return false;

  if (pbtRx != NULL) {
    // Save the received byte count
    *pszRx = szRx - 1;

    // Copy the received bytes
    memcpy (pbtRx, abtRx + 1, *pszRx);
  }
  // Everything went successful
  return true;
}

void __pn53x_init_timer(nfc_device_t * pnd, const uint32_t max_cycles)
{
// The prescaler will dictate what will be the precision and
// the largest delay to measure before saturation. Some examples:
// prescaler =  0 => precision:  ~73ns  timer saturates at    ~5ms
// prescaler =  1 => precision: ~221ns  timer saturates at   ~15ms
// prescaler =  2 => precision: ~369ns  timer saturates at   ~25ms
// prescaler = 10 => precision: ~1.5us  timer saturates at  ~100ms
  if (max_cycles > 0xFFFF) {
      CHIP_DATA (pnd)->timer_prescaler = ((max_cycles/0xFFFF)-1)/2;
  } else {
      CHIP_DATA (pnd)->timer_prescaler = 0;
  }
  uint16_t reloadval = 0xFFFF;
  // Initialize timer
  pn53x_write_register (pnd, PN53X_REG_CIU_TMode, 0xFF, SYMBOL_TAUTO | ((CHIP_DATA (pnd)->timer_prescaler >> 8) & SYMBOL_TPRESCALERHI));
  pn53x_write_register (pnd, PN53X_REG_CIU_TPrescaler, 0xFF, (CHIP_DATA (pnd)->timer_prescaler & SYMBOL_TPRESCALERLO));
  pn53x_write_register (pnd, PN53X_REG_CIU_TReloadVal_hi, 0xFF, (reloadval >> 8) & 0xFF);
  pn53x_write_register (pnd, PN53X_REG_CIU_TReloadVal_lo, 0xFF, reloadval & 0xFF);
}

uint32_t __pn53x_get_timer(nfc_device_t * pnd, const uint8_t last_cmd_byte)
{
  uint8_t parity;
  uint8_t counter_hi, counter_lo;
  uint16_t counter, u16cycles;
  uint32_t u32cycles;
  size_t off = 0;
  if (CHIP_DATA(pnd)->type == PN533) {
    // PN533 prepends its answer by a status byte
    off = 1;
  }
  // Read timer
  BUFFER_INIT (abtReadRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
  BUFFER_APPEND (abtReadRegisterCmd, ReadRegister);
  BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_TCounterVal_hi  >> 8);
  BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_TCounterVal_hi & 0xff);
  BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_TCounterVal_lo  >> 8);
  BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_TCounterVal_lo & 0xff);
  uint8_t abtRes[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t szRes = sizeof(abtRes);
  // Let's send the previously constructed ReadRegister command
  if (!pn53x_transceive (pnd, abtReadRegisterCmd, BUFFER_SIZE (abtReadRegisterCmd), abtRes, &szRes)) {
    return false;
  }
  counter_hi = abtRes[off];
  counter_lo = abtRes[off+1];
  counter = counter_hi;
  counter = (counter << 8) + counter_lo;
  if (counter == 0) {
    // counter saturated
    u32cycles = 0xFFFFFFFF;
  } else {
    u16cycles = 0xFFFF - counter;
    u32cycles = u16cycles;
    u32cycles *= (CHIP_DATA (pnd)->timer_prescaler * 2 + 1);
    u32cycles++;
    // Correction depending on PN53x Rx detection handling:
    // timer stops after 5 (or 2 for PN531) bits are received
    if(CHIP_DATA(pnd)->type == PN531) {
      u32cycles -= (2 * 128);
    } else {
      u32cycles -= (5 * 128);
    }
    // Correction depending on last parity bit sent
    parity = (last_cmd_byte >> 7) ^ ((last_cmd_byte >> 6) & 1) ^
    ((last_cmd_byte >> 5) & 1) ^ ((last_cmd_byte >> 4) & 1) ^
    ((last_cmd_byte >> 3) & 1) ^ ((last_cmd_byte >> 2) & 1) ^
    ((last_cmd_byte >> 1) & 1) ^ (last_cmd_byte & 1);
    parity = parity ? 0:1;
    // When sent ...YY (cmd ends with logical 1, so when last parity bit is 1):
    if (parity) {
      // it finishes 64us sooner than a ...ZY signal
      u32cycles += 64;
    }
    // Correction depending on device design
    u32cycles += CHIP_DATA(pnd)->timer_correction;
  }
  return u32cycles;
}

bool
pn53x_initiator_transceive_bits_timed (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits,
                                       const byte_t * pbtTxPar, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar, uint32_t * cycles)
{
  // TODO Do something with these bytes...
  (void) pbtTxPar;
  (void) pbtRxPar;
  uint16_t i;
  uint8_t sz;

  // Sorry, no arbitrary parity bits support for now
  if (!pnd->bPar) {
    pnd->iLastError = DENOTSUP;
    return false;
  }
  // Sorry, no easy framing support
  if (pnd->bEasyFraming) {
    pnd->iLastError = DENOTSUP;
    return false;
  }
  // Sorry, no CRC support
  // TODO but it probably doesn't make sense for (szTxBits % 8 != 0) ...
  if (pnd->bCrc) {
    pnd->iLastError = DENOTSUP;
    return false;
  }

  __pn53x_init_timer(pnd, *cycles);

  // Once timer is started, we cannot use Tama commands anymore.
  // E.g. on SCL3711 timer settings are reset by 0x42 InCommunicateThru command to:
  //  631a=82 631b=a5 631c=02 631d=00
  // Prepare FIFO
  BUFFER_INIT (abtWriteRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
  BUFFER_APPEND (abtWriteRegisterCmd, WriteRegister);

  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_Command  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_Command & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_COMMAND & SYMBOL_COMMAND_TRANSCEIVE);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOLevel  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOLevel & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_FLUSH_BUFFER);
  for (i=0; i< ((szTxBits / 8) + 1); i++) {
    BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOData  >> 8);
    BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOData & 0xff);
    BUFFER_APPEND (abtWriteRegisterCmd, pbtTx[i]);
  }
  // Send data
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_BitFraming  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_BitFraming & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_START_SEND | ((szTxBits % 8) & SYMBOL_TX_LAST_BITS));
  // Let's send the previously constructed WriteRegister command
  if (!pn53x_transceive (pnd, abtWriteRegisterCmd, BUFFER_SIZE (abtWriteRegisterCmd), NULL, NULL)) {
    return false;
  }

  // Recv data
  *pszRxBits = 0;
  // we've to watch for coming data until we decide to timeout.
  // our PN53x timer saturates after 4.8ms so this function shouldn't be used for
  // responses coming very late anyway.
  // Ideally we should implement a real timer here too but looping a few times is good enough.
  for (i=0; i<(3 *(CHIP_DATA (pnd)->timer_prescaler * 2 + 1)); i++) {
    pn53x_read_register (pnd, PN53X_REG_CIU_FIFOLevel, &sz);
    if (sz > 0)
      break;
  }
  size_t off = 0;
  if (CHIP_DATA(pnd)->type == PN533) {
    // PN533 prepends its answer by a status byte
    off = 1;
  }
  while (1) {
    BUFFER_INIT (abtReadRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
    BUFFER_APPEND (abtReadRegisterCmd, ReadRegister);
    for (i=0; i<sz; i++) {
      BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOData  >> 8);
      BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOData & 0xff);
    }
    BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOLevel  >> 8);
    BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOLevel & 0xff);
    uint8_t abtRes[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
    size_t szRes = sizeof(abtRes);
    // Let's send the previously constructed ReadRegister command
    if (!pn53x_transceive (pnd, abtReadRegisterCmd, BUFFER_SIZE (abtReadRegisterCmd), abtRes, &szRes)) {
      return false;
    }
    for (i = 0; i < sz; i++) {
      pbtRx[i+*pszRxBits] = abtRes[i+off];
    }
    *pszRxBits += (size_t) (sz & SYMBOL_FIFO_LEVEL);
    sz = abtRes[sz+off];
    if (sz == 0)
      break;
  }
  *pszRxBits *= 8; // in bits, not bytes

  // Recv corrected timer value
  *cycles = __pn53x_get_timer (pnd, pbtTx[szTxBits / 8]);

  return true;
}

bool
pn53x_initiator_transceive_bytes_timed (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                                        size_t * pszRx, uint32_t * cycles)
{
  uint16_t i;
  uint8_t sz;

  // We can not just send bytes without parity while the PN53X expects we handled them
  if (!pnd->bPar) {
    pnd->iLastError = DENOTSUP;
    return false;
  }
  // Sorry, no easy framing support
  // TODO: to be changed once we'll provide easy framing support from libnfc itself...
  if (pnd->bEasyFraming) {
    pnd->iLastError = DENOTSUP;
    return false;
  }

  __pn53x_init_timer(pnd, *cycles);

  // Once timer is started, we cannot use Tama commands anymore.
  // E.g. on SCL3711 timer settings are reset by 0x42 InCommunicateThru command to:
  //  631a=82 631b=a5 631c=02 631d=00
  // Prepare FIFO
  BUFFER_INIT (abtWriteRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
  BUFFER_APPEND (abtWriteRegisterCmd, WriteRegister);

  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_Command  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_Command & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_COMMAND & SYMBOL_COMMAND_TRANSCEIVE);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOLevel  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOLevel & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_FLUSH_BUFFER);
  for (i=0; i< szTx; i++) {
    BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOData  >> 8);
    BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_FIFOData & 0xff);
    BUFFER_APPEND (abtWriteRegisterCmd, pbtTx[i]);
  }
  // Send data
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_BitFraming  >> 8);
  BUFFER_APPEND (abtWriteRegisterCmd, PN53X_REG_CIU_BitFraming & 0xff);
  BUFFER_APPEND (abtWriteRegisterCmd, SYMBOL_START_SEND);
  // Let's send the previously constructed WriteRegister command
  if (!pn53x_transceive (pnd, abtWriteRegisterCmd, BUFFER_SIZE (abtWriteRegisterCmd), NULL, NULL)) {
    return false;
  }

  // Recv data
  *pszRx = 0;
  // we've to watch for coming data until we decide to timeout.
  // our PN53x timer saturates after 4.8ms so this function shouldn't be used for
  // responses coming very late anyway.
  // Ideally we should implement a real timer here too but looping a few times is good enough.
  for (i=0; i<(3 *(CHIP_DATA (pnd)->timer_prescaler * 2 + 1)); i++) {
    pn53x_read_register (pnd, PN53X_REG_CIU_FIFOLevel, &sz);
    if (sz > 0)
      break;
  }
  size_t off = 0;
  if (CHIP_DATA(pnd)->type == PN533) {
    // PN533 prepends its answer by a status byte
    off = 1;
  }
  while (1) {
    BUFFER_INIT (abtReadRegisterCmd, PN53x_EXTENDED_FRAME__DATA_MAX_LEN);
    BUFFER_APPEND (abtReadRegisterCmd, ReadRegister);
    for (i=0; i<sz; i++) {
      BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOData  >> 8);
      BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOData & 0xff);
    }
    BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOLevel  >> 8);
    BUFFER_APPEND (abtReadRegisterCmd, PN53X_REG_CIU_FIFOLevel & 0xff);
    uint8_t abtRes[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
    size_t szRes = sizeof(abtRes);
    // Let's send the previously constructed ReadRegister command
    if (!pn53x_transceive (pnd, abtReadRegisterCmd, BUFFER_SIZE (abtReadRegisterCmd), abtRes, &szRes)) {
      return false;
    }
    for (i = 0; i < sz; i++) {
      pbtRx[i+*pszRx] = abtRes[i+off];
    }
    *pszRx += (size_t) (sz & SYMBOL_FIFO_LEVEL);
    sz = abtRes[sz+off];
    if (sz == 0)
      break;
  }

  // Recv corrected timer value
  if (pnd->bCrc) {
    // We've to compute CRC ourselves to know last byte actually sent
    uint8_t * pbtTxRaw;
    pbtTxRaw = (uint8_t *) malloc(szTx+2);
    memcpy (pbtTxRaw, pbtTx, szTx);
    iso14443a_crc_append (pbtTxRaw, szTx);
    *cycles = __pn53x_get_timer (pnd, pbtTxRaw[szTx +1]);
    free(pbtTxRaw);
  } else {
    *cycles = __pn53x_get_timer (pnd, pbtTx[szTx -1]);
  }
  return true;
}

bool
pn53x_initiator_deselect_target (nfc_device_t * pnd)
{
  return (pn53x_InDeselect (pnd, 0));   // 0 mean deselect all selected targets
}

#define SAK_ISO14443_4_COMPLIANT 0x20
#define SAK_ISO18092_COMPLIANT   0x40
bool
pn53x_target_init (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx)
{
  pn53x_reset_settings(pnd);

  CHIP_DATA (pnd)->operating_mode = TARGET;

  pn53x_target_mode_t ptm = PTM_NORMAL;

  switch (pnt->nm.nmt) {
    case NMT_ISO14443A:
      ptm = PTM_PASSIVE_ONLY;
      if ((pnt->nti.nai.abtUid[0] != 0x08) || (pnt->nti.nai.szUidLen != 4)) {
        pnd->iLastError = ETGUIDNOTSUP;
        return false;
      }
      pn53x_set_parameters (pnd, PARAM_AUTO_ATR_RES, false);
      if (CHIP_DATA(pnd)->type == PN532) { // We have a PN532
        if ((pnt->nti.nai.btSak & SAK_ISO14443_4_COMPLIANT) && (pnd->bAutoIso14443_4)) {
          // We have a ISO14443-4 tag to emulate and NDO_AUTO_14443_4A option is enabled
          ptm |= PTM_ISO14443_4_PICC_ONLY; // We add ISO14443-4 restriction
          pn53x_set_parameters (pnd, PARAM_14443_4_PICC, true);
        } else {
          pn53x_set_parameters (pnd, PARAM_14443_4_PICC, false);
        }
      }
    break;
    case NMT_FELICA:
      ptm = PTM_PASSIVE_ONLY;
    break;
    case NMT_DEP:
      pn53x_set_parameters (pnd, PARAM_AUTO_ATR_RES, true);
      ptm = PTM_DEP_ONLY;
      if (pnt->nti.ndi.ndm == NDM_PASSIVE) {
        ptm |= PTM_PASSIVE_ONLY; // We add passive mode restriction
      }
    break;
    case NMT_ISO14443B:
    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
    case NMT_JEWEL:
      pnd->iLastError = DENOTSUP;
      return false;
    break;
  }

  // Let the PN53X be activated by the RF level detector from power down mode
  if (!pn53x_write_register (pnd, PN53X_REG_CIU_TxAuto, SYMBOL_INITIAL_RF_ON, 0x04))
    return false;

  byte_t abtMifareParams[6];
  byte_t * pbtMifareParams = NULL;
  byte_t * pbtTkt = NULL;
  size_t szTkt = 0;

  byte_t abtFeliCaParams[18];
  byte_t * pbtFeliCaParams = NULL;

  const byte_t * pbtNFCID3t = NULL;
  const byte_t * pbtGBt = NULL;
  size_t szGBt = 0;

  switch(pnt->nm.nmt) {
    case NMT_ISO14443A: {
      // Set ATQA (SENS_RES)
      abtMifareParams[0] = pnt->nti.nai.abtAtqa[1];
      abtMifareParams[1] = pnt->nti.nai.abtAtqa[0];
      // Set UID
      // Note: in this mode we can only emulate a single size (4 bytes) UID where the first is hard-wired by PN53x as 0x08
      abtMifareParams[2] = pnt->nti.nai.abtUid[1];
      abtMifareParams[3] = pnt->nti.nai.abtUid[2];
      abtMifareParams[4] = pnt->nti.nai.abtUid[3];
      // Set SAK (SEL_RES)
      abtMifareParams[5] = pnt->nti.nai.btSak;

      pbtMifareParams = abtMifareParams;

      // Historical Bytes
      pbtTkt = iso14443a_locate_historical_bytes (pnt->nti.nai.abtAts, pnt->nti.nai.szAtsLen, &szTkt);
    }
    break;

    case NMT_FELICA:
      // Set NFCID2t
      memcpy(abtFeliCaParams, pnt->nti.nfi.abtId, 8);
      // Set PAD
      memcpy(abtFeliCaParams+8, pnt->nti.nfi.abtPad, 8);
      // Set SystemCode
      memcpy(abtFeliCaParams+16, pnt->nti.nfi.abtSysCode, 2);
      pbtFeliCaParams = abtFeliCaParams;
    break;

    case NMT_DEP:
      // Set NFCID3
      pbtNFCID3t = pnt->nti.ndi.abtNFCID3;
      // Set General Bytes, if relevant
      szGBt = pnt->nti.ndi.szGB;
      if (szGBt) pbtGBt = pnt->nti.ndi.abtGB;

      // Set ISO/IEC 14443 part
      // Set ATQA (SENS_RES)
      abtMifareParams[0] = 0x08;
      abtMifareParams[1] = 0x00;
      // Set UID
      // Note: in this mode we can only emulate a single size (4 bytes) UID where the first is hard-wired by PN53x as 0x08
      abtMifareParams[2] = 0x12;
      abtMifareParams[3] = 0x34;
      abtMifareParams[4] = 0x56;
      // Set SAK (SEL_RES)
      abtMifareParams[5] = SAK_ISO18092_COMPLIANT; // Allow ISO/IEC 18092 in DEP mode

      pbtMifareParams = abtMifareParams;

      // Set FeliCa part
      // Set NFCID2t
      abtFeliCaParams[0] = 0x01;
      abtFeliCaParams[1] = 0xfe;
      abtFeliCaParams[2] = 0x12;
      abtFeliCaParams[3] = 0x34;
      abtFeliCaParams[4] = 0x56;
      abtFeliCaParams[5] = 0x78;
      abtFeliCaParams[6] = 0x90;
      abtFeliCaParams[7] = 0x12;
      // Set PAD
      abtFeliCaParams[8]  = 0xc0;
      abtFeliCaParams[9]  = 0xc1;
      abtFeliCaParams[10] = 0xc2;
      abtFeliCaParams[11] = 0xc3;
      abtFeliCaParams[12] = 0xc4;
      abtFeliCaParams[13] = 0xc5;
      abtFeliCaParams[14] = 0xc6;
      abtFeliCaParams[15] = 0xc7;
      // Set System Code
      abtFeliCaParams[16] = 0x0f;
      abtFeliCaParams[17] = 0xab;
    
      pbtFeliCaParams = abtFeliCaParams;
    break;
    case NMT_ISO14443B:
    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
    case NMT_JEWEL:
      pnd->iLastError = DENOTSUP;
      return false;
    break;
  }

  bool targetActivated = false;
  while (!targetActivated) {
    byte_t btActivatedMode;

    if(!pn53x_TgInitAsTarget(pnd, ptm, pbtMifareParams, pbtTkt, szTkt, pbtFeliCaParams, pbtNFCID3t, pbtGBt, szGBt, pbtRx, pszRx, &btActivatedMode)) {
      return false;
    }

    nfc_modulation_t nm = {
      .nmt = NMT_DEP, // Silent compilation warnings
      .nbr = NBR_UNDEFINED
    };
    nfc_dep_mode_t ndm = NDM_UNDEFINED;
    // Decode activated "mode"
    switch(btActivatedMode & 0x70) { // Baud rate
      case 0x00: // 106kbps
        nm.nbr = NBR_106;
      break;
      case 0x10: // 212kbps
        nm.nbr = NBR_212;
      break;
      case 0x20: // 424kbps
        nm.nbr = NBR_424;
      break;
    };

    if (btActivatedMode & 0x04) { // D.E.P.
      nm.nmt = NMT_DEP;
      if ((btActivatedMode & 0x03) == 0x01) { // Active mode
        ndm = NDM_ACTIVE;
      } else { // Passive mode
        ndm = NDM_PASSIVE;
      }
    } else { // Not D.E.P.
      if ((btActivatedMode & 0x03) == 0x00) { // MIFARE
        nm.nmt = NMT_ISO14443A;
      } else if ((btActivatedMode & 0x03) == 0x02) { // FeliCa
        nm.nmt = NMT_FELICA;
      }
    }

    if(pnt->nm.nmt == nm.nmt) { // Actual activation have the right modulation type
      if ((pnt->nm.nbr == NBR_UNDEFINED) || (pnt->nm.nbr == nm.nbr)) { // Have the right baud rate (or undefined)
        if ((pnt->nm.nmt != NMT_DEP) || (pnt->nti.ndi.ndm == NDM_UNDEFINED) || (pnt->nti.ndi.ndm == ndm)) { // Have the right DEP mode (or is not a DEP)
          targetActivated = true;
        }
      }
    }

    if (targetActivated) {
      pnt->nm.nbr = nm.nbr; // Update baud rate
      if (pnt->nm.nmt == NMT_DEP) {
        pnt->nti.ndi.ndm = ndm; // Update DEP mode
      }
      // Keep the current nfc_target for further commands
      if (CHIP_DATA (pnd)->current_target) {
        free (CHIP_DATA (pnd)->current_target);
      }
      CHIP_DATA (pnd)->current_target = malloc (sizeof(nfc_target_t));
      memcpy (CHIP_DATA (pnd)->current_target, pnt, sizeof(nfc_target_t));
    }
  }

  return true;
}

bool
pn53x_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  byte_t  abtCmd[] = { TgGetInitiatorCommand };

  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof (abtRx);
  // Try to gather a received frame from the reader
  if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), abtRx, &szRx))
    return false;

  // Get the last bit-count that is stored in the received byte
  uint8_t ui8rcc;
  if (!pn53x_read_register (pnd, PN53X_REG_CIU_Control, &ui8rcc))
    return false;
  uint8_t ui8Bits = ui8rcc & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  size_t szFrameBits = ((szRx - 1 - ((ui8Bits == 0) ? 0 : 1)) * 8) + ui8Bits;

  // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
  // Check if we should recover the parity bits ourself
  if (!pnd->bPar) {
    // Unwrap the response frame
    pn53x_unwrap_frame (abtRx + 1, szFrameBits, pbtRx, pszRxBits, pbtRxPar);
  } else {
    // Save the received bits
    *pszRxBits = szFrameBits;
    // Copy the received bytes
    memcpy (pbtRx, abtRx + 1, szRx - 1);
  }
  // Everyting seems ok, return true
  return true;
}

bool
pn53x_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx)
{
  byte_t  abtCmd[1];

  if (pnd->bEasyFraming) {
    if ((CHIP_DATA (pnd)->current_target->nm.nmt == NMT_DEP) || // If DEP mode
    ((CHIP_DATA(pnd)->type == PN532) && (pnd->bAutoIso14443_4) &&
    (CHIP_DATA (pnd)->current_target->nm.nmt == NMT_ISO14443A) && (CHIP_DATA (pnd)->current_target->nti.nai.btSak & SAK_ISO14443_4_COMPLIANT)) // If ISO14443-4 PICC emulation
    ) {
      abtCmd[0] = TgGetData;
    } else {
      // TODO Support EasyFraming for other cases by software
      pnd->iLastError = DENOTSUP;
      return false;
    }
  } else {
    abtCmd[0] = TgGetInitiatorCommand;
  }

  // Try to gather a received frame from the reader
  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof (abtRx);
  if (!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), abtRx, &szRx))
    return false;

  // Save the received byte count
  *pszRx = szRx - 1;

  // FIXME szRx can be 0

  // Copy the received bytes
  memcpy (pbtRx, abtRx + 1, *pszRx);

  // Everyting seems ok, return true
  return true;
}

bool
pn53x_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar)
{
  size_t  szFrameBits = 0;
  size_t  szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t  abtCmd[PN53x_EXTENDED_FRAME__DATA_MAX_LEN] = { TgResponseToInitiator };

  // Check if we should prepare the parity bits ourself
  if (!pnd->bPar) {
    // Convert data with parity to a frame
    pn53x_wrap_frame (pbtTx, szTxBits, pbtTxPar, abtCmd + 1, &szFrameBits);
  } else {
    szFrameBits = szTxBits;
  }

  // Retrieve the leading bits
  ui8Bits = szFrameBits % 8;

  // Get the amount of frame bytes + optional (1 byte if there are leading bits)
  szFrameBytes = (szFrameBits / 8) + ((ui8Bits == 0) ? 0 : 1);

  // When the parity is handled before us, we just copy the data
  if (pnd->bPar)
    memcpy (abtCmd + 1, pbtTx, szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits (pnd, ui8Bits))
    return false;

  // Try to send the bits to the reader
  if (!pn53x_transceive (pnd, abtCmd, szFrameBytes + 1, NULL, NULL))
    return false;

  // Everyting seems ok, return true
  return true;
}

bool
pn53x_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx)
{
  byte_t  abtCmd[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar)
    return false;

  if (pnd->bEasyFraming) {
    if ((CHIP_DATA (pnd)->current_target->nm.nmt == NMT_DEP) || // If DEP mode
    ((CHIP_DATA(pnd)->type == PN532) && (pnd->bAutoIso14443_4) &&
    (CHIP_DATA (pnd)->current_target->nm.nmt == NMT_ISO14443A) && (CHIP_DATA (pnd)->current_target->nti.nai.btSak & SAK_ISO14443_4_COMPLIANT)) // If ISO14443-4 PICC emulation
    ) {
      abtCmd[0] = TgSetData;
    } else {
      // TODO Support EasyFraming for other cases by software
      pnd->iLastError = DENOTSUP;
      return false;
    }
  } else {
    abtCmd[0] = TgResponseToInitiator;
  }

  // Copy the data into the command frame
  memcpy (abtCmd + 1, pbtTx, szTx);

  // Try to send the bits to the reader
  if (!pn53x_transceive (pnd, abtCmd, szTx + 1, NULL, NULL))
    return false;

  // Everyting seems ok, return true
  return true;
}

static struct sErrorMessage {
  int     iErrorCode;
  const char *pcErrorMsg;
} sErrorMessages[] = {
  /* Chip-level errors */
  { 0x00, "Success" },
  { ETIMEOUT, "Timeout" },      // Time Out, the target has not answered
  { ECRC,     "CRC Error" },      // A CRC error has been detected by the CIU
  { EPARITY,  "Parity Error" },     // A Parity error has been detected by the CIU
  { EBITCOUNT, "Erroneous Bit Count" },   // During an anti-collision/select operation (ISO/IEC14443-3 Type A and ISO/IEC18092 106 kbps passive mode), an erroneous Bit Count has been detected
  { EFRAMING, "Framing Error" },    // Framing error during MIFARE operation
  { EBITCOLL, "Bit-collision" },    // An abnormal bit-collision has been detected during bit wise anti-collision at 106 kbps
  { ESMALLBUF, "Communication Buffer Too Small" },  // Communication buffer size insufficient
  { EBUFOVF, "Buffer Overflow" },   // RF Buffer overflow has been detected by the CIU (bit BufferOvfl of the register CIU_Error)
  { ERFTIMEOUT, "RF Timeout" },     // In active communication mode, the RF field has not been switched on in time by the counterpart (as defined in NFCIP-1 standard)
  { ERFPROTO, "RF Protocol Error" },    // RF Protocol error (see PN53x manual)
  { EOVHEAT, "Chip Overheating" },    // Temperature error: the internal temperature sensor has detected overheating, and therefore has automatically switched off the antenna drivers
  { EINBUFOVF, "Internal Buffer overflow."},  // Internal buffer overflow
  { EINVPARAM, "Invalid Parameter"},    // Invalid parameter (range, format, …)
  /* DEP Errors */
  { EDEPUNKCMD, "Unknown DEP Command" },
  /* MIFARE */
  { EMFAUTH, "Mifare Authentication Error" },
  /*  */
  { EINVRXFRAM, "Invalid Received Frame" }, // DEP Protocol, Mifare or ISO/IEC14443-4: The data format does not match to the specification.
  { ENSECNOTSUPP, "NFC Secure not supported" }, // Target or Initiator does not support NFC Secure
  { EBCC, "Wrong UID Check Byte (BCC)" }, // ISO/IEC14443-3: UID Check byte is wrong
  { EDEPINVSTATE, "Invalid DEP State" },  // DEP Protocol: Invalid device state, the system is in a state which does not allow the operation
  { EOPNOTALL, "Operation Not Allowed" }, // Operation not allowed in this configuration (host controller interface)
  { ECMD, "Command Not Acceptable" },   // Command is not acceptable due to the current context
  { ETGREL, "Target Released" },    // Target have been released by initiator
  // FIXME: Errors can be grouped (DEP-related, MIFARE-related, ISO14443B-related, etc.)
  // Purposal: Use prefix/suffix to identify them
  { ECID, "Card ID Mismatch" },     // ISO14443 type B: Card ID mismatch, meaning that the expected card has been exchanged with another one.
  { ECDISCARDED, "Card Discarded" },    // ISO/IEC14443 type B: the card previously activated has disappeared.
  { ENFCID3, "NFCID3 Mismatch" },
  { EOVCURRENT, "Over Current"  },
  { ENAD, "NAD Missing in DEP Frame" },
  /* Software level errors */
  { ETGUIDNOTSUP, "Target UID not supported" }, // In target mode, PN53x only support 4 bytes UID and the first byte must start with 0x08
  /* Driver-level errors */
  { DENACK, "Received NACK" },
  { DEACKMISMATCH, "Expected ACK/NACK" },
  { DEISERRFRAME, "Received an error frame" },
  // TODO: Move me in more generic code for libnfc 1.6
  // FIXME: Driver-errors and Device-errors have the same prefix (DE*)
  // eg. DENACK means Driver Error NACK while DEIO means Device Error I/O
  { DEINVAL, "Invalid argument" },
  { DEIO, "Input/output error" },
  { DETIMEOUT, "Operation timed-out" },
  { DEABORT, "Operation aborted" },
  { DENOTSUP, "Operation not supported" }
};

const char *
pn53x_strerror (const nfc_device_t * pnd)
{
  const char *pcRes = "Unknown error";
  size_t  i;

  for (i = 0; i < (sizeof (sErrorMessages) / sizeof (struct sErrorMessage)); i++) {
    if (sErrorMessages[i].iErrorCode == pnd->iLastError) {
      pcRes = sErrorMessages[i].pcErrorMsg;
      break;
    }
  }

  return pcRes;
}

bool
pn53x_SetParameters (nfc_device_t * pnd, const uint8_t ui8Value)
{
  byte_t  abtCmd[] = { SetParameters, ui8Value };

  if(!pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL)) {
    return false;
  }
  // We save last parameters in register cache
  CHIP_DATA (pnd)->ui8Parameters = ui8Value;
  return true;
}

/*
 typedef enum {
     NORMAL = 0x01,
     VIRTUAL_CARD = 0x02,
     WIRED_MODE = 0x03,
     DUAL_CARD = 0x04
 } pn532_sam_mode;
 */

bool
pn53x_SAMConfiguration (nfc_device_t * pnd, const uint8_t ui8Mode)
{
  byte_t  abtCmd[] = { SAMConfiguration, ui8Mode, 0x00, 0x00 };
  size_t szCmd = sizeof(abtCmd);
  switch (ui8Mode) {
    case 0x01: // Normal mode
      szCmd = 2;
      break;
    case 0x02: // Virtual card mode
    case 0x03: // Wired card mode
    case 0x04: // Dual card mode
    // TODO: Handle these SAM mode
      break;
    default:
      pnd->iLastError = DENOTSUP;
      return false;
  }
  return (pn53x_transceive (pnd, abtCmd, szCmd, NULL, NULL));
}

bool
pn53x_PowerDown (nfc_device_t * pnd)
{
  byte_t  abtCmd[] = { PowerDown, 0xf0 };
  return (pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL));
}

/**
 * @brief C wrapper to InListPassiveTarget command
 * @return true if command is successfully sent
 *
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param pmInitModulation Desired modulation
 * @param pbtInitiatorData Optional initiator data used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID
 * @param szInitiatorData Length of initiator data \a pbtInitiatorData
 * @param pbtTargetsData pointer on a pre-allocated byte array to receive TargetData[n] as described in pn53x user manual
 * @param pszTargetsData size_t pointer where size of \a pbtTargetsData will be written
 *
 * @note Selected targets count can be found in \a pbtTargetsData[0] if available (i.e. \a pszTargetsData content is more than 0)
 * @note To decode theses TargetData[n], there is @fn pn53x_decode_target_data
 */
bool
pn53x_InListPassiveTarget (nfc_device_t * pnd,
                           const pn53x_modulation_t pmInitModulation, const byte_t szMaxTargets,
                           const byte_t * pbtInitiatorData, const size_t szInitiatorData,
                           byte_t * pbtTargetsData, size_t * pszTargetsData)
{
  byte_t  abtCmd[15] = { InListPassiveTarget };

  abtCmd[1] = szMaxTargets;     // MaxTg

  // XXX Is there a better way to do handle supported modulations ?
  switch(pmInitModulation) {
    case PM_ISO14443A_106:
    case PM_FELICA_212:
    case PM_FELICA_424:
      // all gone fine.
      break;
    case PM_ISO14443B_106:
      if (!(pnd->btSupportByte & SUPPORT_ISO14443B)) {
        // Eg. Some PN532 doesn't support type B!
        pnd->iLastError = DENOTSUP;
        return false;
      }
      break;
    case PM_JEWEL_106:
      if(CHIP_DATA(pnd)->type == PN531) {
        // These modulations are not supported by pn531
        pnd->iLastError = DENOTSUP;
        return false;
      }
      break;
    case PM_ISO14443B_212:
    case PM_ISO14443B_424:
    case PM_ISO14443B_847:
      if((CHIP_DATA(pnd)->type != PN533) || (!(pnd->btSupportByte & SUPPORT_ISO14443B))) {
        // These modulations are not supported by pn531 neither pn532
        pnd->iLastError = DENOTSUP;
        return false;
      }
      break;
    default:
      pnd->iLastError = DENOTSUP;
      return false;
  }
  abtCmd[2] = pmInitModulation; // BrTy, the type of init modulation used for polling a passive tag

  // Set the optional initiator data (used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID).
  if (pbtInitiatorData)
    memcpy (abtCmd + 3, pbtInitiatorData, szInitiatorData);

  return pn53x_transceive (pnd, abtCmd, 3 + szInitiatorData, pbtTargetsData, pszTargetsData);
}

bool
pn53x_InDeselect (nfc_device_t * pnd, const uint8_t ui8Target)
{
  if (CHIP_DATA(pnd)->type == RCS360) {
    // We should do act here *only* if a target was previsouly selected
    byte_t  abtStatus[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
    size_t  szStatus = sizeof(abtStatus);
    byte_t  abtCmdGetStatus[] = { GetGeneralStatus };
    if (!pn53x_transceive (pnd, abtCmdGetStatus, sizeof (abtCmdGetStatus), abtStatus, &szStatus)) {
      return false;
    }
    if ((szStatus < 3) || (abtStatus[2] == 0)) {
      return true;
    }
    // No much choice what to deselect actually...
    byte_t  abtCmdRcs360[] = { InDeselect, 0x01, 0x01 };
    return (pn53x_transceive (pnd, abtCmdRcs360, sizeof (abtCmdRcs360), NULL, NULL));
  }
  byte_t  abtCmd[] = { InDeselect, ui8Target };
  return (pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL));
}

bool
pn53x_InRelease (nfc_device_t * pnd, const uint8_t ui8Target)
{
  if (CHIP_DATA(pnd)->type == RCS360) {
    // We should do act here *only* if a target was previsouly selected
    byte_t  abtStatus[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
    size_t  szStatus = sizeof(abtStatus);
    byte_t  abtCmdGetStatus[] = { GetGeneralStatus };
    if (!pn53x_transceive (pnd, abtCmdGetStatus, sizeof (abtCmdGetStatus), abtStatus, &szStatus)) {
      return false;
    }
    if ((szStatus < 3) || (abtStatus[2] == 0)) {
      return true;
    }
    // No much choice what to release actually...
    byte_t  abtCmdRcs360[] = { InRelease, 0x01, 0x01 };
    return (pn53x_transceive (pnd, abtCmdRcs360, sizeof (abtCmdRcs360), NULL, NULL));
  }
  byte_t  abtCmd[] = { InRelease, ui8Target };
  return (pn53x_transceive (pnd, abtCmd, sizeof (abtCmd), NULL, NULL));
}

bool
pn53x_InAutoPoll (nfc_device_t * pnd,
                  const pn53x_target_type_t * ppttTargetTypes, const size_t szTargetTypes,
                  const byte_t btPollNr, const byte_t btPeriod, nfc_target_t * pntTargets, size_t * pszTargetFound)
{
  if (CHIP_DATA(pnd)->type != PN532) {
    // This function is not supported by pn531 neither pn533
    pnd->iLastError = DENOTSUP;
    return false;
  }

  // InAutoPoll frame looks like this { 0xd4, 0x60, 0x0f, 0x01, 0x00 } => { direction, command, pollnr, period, types... }
  size_t szTxInAutoPoll = 3 + szTargetTypes;
  byte_t abtCmd[3+15] = { InAutoPoll, btPollNr, btPeriod };
  for (size_t n = 0; n < szTargetTypes; n++) {
    abtCmd[3 + n] = ppttTargetTypes[n];
  }

  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof(abtRx);
  bool res = pn53x_transceive (pnd, abtCmd, szTxInAutoPoll, abtRx, &szRx);

  if (res == false) {
    return false;
  } else if (szRx > 0) {
    *pszTargetFound = abtRx[0];
    if (*pszTargetFound) {
      uint8_t ln;
      byte_t *pbt = abtRx + 1;
      /* 1st target */
      // Target type
      pn53x_target_type_t ptt = *(pbt++);
      pntTargets[0].nm = pn53x_ptt_to_nm(ptt);
      // AutoPollTargetData length
      ln = *(pbt++);
      pn53x_decode_target_data (pbt, ln, CHIP_DATA(pnd)->type, pntTargets[0].nm.nmt, &(pntTargets[0].nti));
      pbt += ln;

      if (abtRx[0] > 1) {
        /* 2nd target */
        // Target type
        ptt = *(pbt++);
        pntTargets[1].nm = pn53x_ptt_to_nm(ptt);
        // AutoPollTargetData length
        ln = *(pbt++);
        pn53x_decode_target_data (pbt, ln, CHIP_DATA(pnd)->type, pntTargets[1].nm.nmt, &(pntTargets[1].nti));
      }
    }
  }
  return true;
}

/**
 * @brief Wrapper for InJumpForDEP command
 * @param pmInitModulation desired initial modulation
 * @param pbtPassiveInitiatorData NFCID1 (4 bytes) at 106kbps (optionnal, see NFCIP-1: 11.2.1.26) or Polling Request Frame's payload (5 bytes) at 212/424kbps (mandatory, see NFCIP-1: 11.2.2.5)
 * @param szPassiveInitiatorData size of pbtPassiveInitiatorData content
 * @param pbtNFCID3i NFCID3 of the initiator
 * @param pbtGBi General Bytes of the initiator
 * @param szGBi count of General Bytes
 * @param[out] pnt \a nfc_target_t which will be filled by this function
 */
bool
pn53x_InJumpForDEP (nfc_device_t * pnd,
                    const nfc_dep_mode_t ndm,
                    const nfc_baud_rate_t nbr,
                    const byte_t * pbtPassiveInitiatorData,
                    const byte_t * pbtNFCID3i,
                    const byte_t * pbtGBi, const size_t szGBi,
                    nfc_target_t * pnt)
{
  byte_t  abtCmd[PN53x_EXTENDED_FRAME__DATA_MAX_LEN] = { InJumpForDEP, (ndm == NDM_ACTIVE) ? 0x01 : 0x00 };

  size_t offset = 4; // 1 byte for command, 1 byte for DEP mode (Active/Passive), 1 byte for baud rate, 1 byte for following parameters flag

  switch (nbr) {
    case NBR_106:
      abtCmd[2] = 0x00; // baud rate is 106 kbps
    break;
    case NBR_212:
      abtCmd[2] = 0x01; // baud rate is 212 kbps
    break;
    case NBR_424:
      abtCmd[2] = 0x02; // baud rate is 424 kbps
    break;
    case NBR_847:
    case NBR_UNDEFINED:
      // XXX Maybe we should put a "syntax error" or sth like that
      pnd->iLastError = DENOTSUP;
      return false;
    break;
  }

  if (pbtPassiveInitiatorData && (ndm == NDM_PASSIVE)) {        /* can't have passive initiator data when using active mode */
    switch (nbr) {
      case NBR_106:
        abtCmd[3] |= 0x01;
        memcpy (abtCmd + offset, pbtPassiveInitiatorData, 4);
        offset += 4;
      break;
      case NBR_212:
      case NBR_424:
        abtCmd[3] |= 0x01;
        memcpy (abtCmd + offset, pbtPassiveInitiatorData, 5);
        offset += 5;
      break;
      case NBR_847:
      case NBR_UNDEFINED:
        // XXX Maybe we should put a "syntax error" or sth like that
        pnd->iLastError = DENOTSUP;
        return false;
      break;
    }
  }

  if (pbtNFCID3i) {
    abtCmd[3] |= 0x02;
    memcpy (abtCmd + offset, pbtNFCID3i, 10);
    offset += 10;
  }

  if (szGBi && pbtGBi) {
    abtCmd[3] |= 0x04;
    memcpy (abtCmd + offset, pbtGBi, szGBi);
    offset += szGBi;
  }

  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t  szRx = sizeof (abtRx);
  // Try to find a target, call the transceive callback function of the current device
  if (!pn53x_transceive (pnd, abtCmd, offset, abtRx, &szRx))
    return false;

  // Make sure one target has been found, the PN53X returns 0x00 if none was available
  if (abtRx[1] != 1)
    return false;

  // Is a target struct available
  if (pnt) {
    pnt->nm.nmt = NMT_DEP;
    pnt->nm.nbr = nbr;
    memcpy (pnt->nti.ndi.abtNFCID3, abtRx + 2, 10);
    pnt->nti.ndi.btDID = abtRx[12];
    pnt->nti.ndi.btBS = abtRx[13];
    pnt->nti.ndi.btBR = abtRx[14];
    pnt->nti.ndi.btTO = abtRx[15];
    pnt->nti.ndi.btPP = abtRx[16];
    if(szRx > 17) {
      pnt->nti.ndi.szGB = szRx - 17;
      memcpy (pnt->nti.ndi.abtGB, abtRx + 17, pnt->nti.ndi.szGB);
    } else {
      pnt->nti.ndi.szGB = 0;
    }
  }
  return true;
}

bool
pn53x_TgInitAsTarget (nfc_device_t * pnd, pn53x_target_mode_t ptm,
                      const byte_t * pbtMifareParams,
                      const byte_t * pbtTkt, size_t szTkt,
                      const byte_t * pbtFeliCaParams,
                      const byte_t * pbtNFCID3t, const byte_t * pbtGBt, const size_t szGBt,
                      byte_t * pbtRx, size_t * pszRx, byte_t * pbtModeByte)
{
  byte_t  abtCmd[39 + 47 + 48] = { TgInitAsTarget }; // Worst case: 39-byte base, 47 bytes max. for General Bytes, 48 bytes max. for Historical Bytes
  size_t  szOptionalBytes = 0;

  // Clear the target init struct, reset to all zeros
  memset (abtCmd + 1, 0x00, sizeof (abtCmd) - 1);

  // Store the target mode in the initialization params
  abtCmd[1] = ptm;

  // MIFARE part
  if (pbtMifareParams) {
    memcpy (abtCmd+2, pbtMifareParams, 6);
  }
  // FeliCa part
  if (pbtFeliCaParams) {
    memcpy (abtCmd+8, pbtFeliCaParams, 18);
  }
  // DEP part
  if (pbtNFCID3t) {
    memcpy(abtCmd+26, pbtNFCID3t, 10);
  }
  // General Bytes (ISO/IEC 18092)
  if (CHIP_DATA(pnd)->type == PN531) {
    if (szGBt) {
      memcpy (abtCmd+36, pbtGBt, szGBt);
      szOptionalBytes = szGBt;
    }
  } else {
    abtCmd[36] = (byte_t)(szGBt);
    if (szGBt) {
      memcpy (abtCmd+37, pbtGBt, szGBt);
    }
    szOptionalBytes = szGBt + 1;
  }
  // Historical bytes (ISO/IEC 14443-4)
  if (CHIP_DATA(pnd)->type != PN531) { // PN531 does not handle Historical Bytes
    abtCmd[36+szOptionalBytes] = (byte_t)(szTkt);
    if (szTkt) {
      memcpy (abtCmd+37+szOptionalBytes, pbtTkt, szTkt);
    }
    szOptionalBytes += szTkt + 1;
  }

  // Request the initialization as a target
  byte_t  abtRx[PN53x_EXTENDED_FRAME__DATA_MAX_LEN];
  size_t szRx = sizeof (abtRx);
  if (!pn53x_transceive (pnd, abtCmd, 36 + szOptionalBytes, abtRx, &szRx))
    return false;

  // Note: the first byte is skip: 
  //       its the "mode" byte which contains baudrate, DEP and Framing type (Mifare, active or FeliCa) datas.
  if(pbtModeByte) {
    *pbtModeByte = abtRx[0];
  }

  // Save the received byte count
  *pszRx = szRx - 1;
  // Copy the received bytes
  memcpy (pbtRx, abtRx + 1, *pszRx);

  return true;
}

bool
pn53x_check_ack_frame (nfc_device_t * pnd, const byte_t * pbtRxFrame, const size_t szRxFrameLen)
{
  if (szRxFrameLen >= sizeof (pn53x_ack_frame)) {
    if (0 == memcmp (pbtRxFrame, pn53x_ack_frame, sizeof (pn53x_ack_frame))) {
      // DBG ("%s", "PN53x ACKed");
      return true;
    }
  }
  pnd->iLastError = DEACKMISMATCH;
  ERR ("%s", "Unexpected PN53x reply!");
  return false;
}

bool
pn53x_check_error_frame (nfc_device_t * pnd, const byte_t * pbtRxFrame, const size_t szRxFrameLen)
{
  if (szRxFrameLen >= sizeof (pn53x_error_frame)) {
    if (0 == memcmp (pbtRxFrame, pn53x_error_frame, sizeof (pn53x_error_frame))) {
      DBG ("%s", "PN53x sent an error frame");
      pnd->iLastError = DEISERRFRAME;
      return false;
    }
  }
  return true;
}

/**
 * @brief Build a PN53x frame
 *
 * @param pbtData payload (bytes array) of the frame, will become PD0, ..., PDn in PN53x frame
 * @note The first byte of pbtData is the Command Code (CC)
 */
bool
pn53x_build_frame (byte_t * pbtFrame, size_t * pszFrame, const byte_t * pbtData, const size_t szData)
{
  if (szData <= PN53x_NORMAL_FRAME__DATA_MAX_LEN) {
    // LEN - Packet length = data length (len) + checksum (1) + end of stream marker (1)
    pbtFrame[3] = szData + 1;
    // LCS - Packet length checksum
    pbtFrame[4] = 256 - (szData + 1);
    // TFI
    pbtFrame[5] = 0xD4;
    // DATA - Copy the PN53X command into the packet buffer
    memcpy (pbtFrame + 6, pbtData, szData);

    // DCS - Calculate data payload checksum
    byte_t btDCS = (256 - 0xD4);
    for (size_t szPos = 0; szPos < szData; szPos++) {
      btDCS -= pbtData[szPos];
    }
    pbtFrame[6 + szData] = btDCS;

    // 0x00 - End of stream marker
    pbtFrame[szData + 7] = 0x00;

    (*pszFrame) = szData + PN53x_NORMAL_FRAME__OVERHEAD;
  } else if (szData <= PN53x_EXTENDED_FRAME__DATA_MAX_LEN) {
    // Extended frame marker
    pbtFrame[3] = 0xff;
    pbtFrame[4] = 0xff;
    // LENm
    pbtFrame[5] = (szData + 1) >> 8;
    // LENl
    pbtFrame[6] = (szData + 1) & 0xff;
    // LCS
    pbtFrame[7] = 256 - ((pbtFrame[5] + pbtFrame[6]) & 0xff);
    // TFI
    pbtFrame[8] = 0xD4;
    // DATA - Copy the PN53X command into the packet buffer
    memcpy (pbtFrame + 9, pbtData, szData);

    // DCS - Calculate data payload checksum
    byte_t btDCS = (256 - 0xD4);
    for (size_t szPos = 0; szPos < szData; szPos++) {
      btDCS -= pbtData[szPos];
    }
    pbtFrame[9 + szData] = btDCS;

    // 0x00 - End of stream marker
    pbtFrame[szData + 10] = 0x00;

    (*pszFrame) = szData + PN53x_EXTENDED_FRAME__OVERHEAD;
  } else {
    ERR ("We can't send more than %d bytes in a raw (requested: %zd)", PN53x_EXTENDED_FRAME__DATA_MAX_LEN, szData);
    return false;
  }
  return true;
}
pn53x_modulation_t
pn53x_nm_to_pm(const nfc_modulation_t nm)
{
  switch(nm.nmt) {
    case NMT_ISO14443A:
      return PM_ISO14443A_106;
    break;

    case NMT_ISO14443B:
      switch(nm.nbr) {
        case NBR_106:
          return PM_ISO14443B_106;
        break;
        case NBR_212:
          return PM_ISO14443B_212;
        break;
        case NBR_424:
          return PM_ISO14443B_424;
        break;
        case NBR_847:
          return PM_ISO14443B_847;
        break;
        case NBR_UNDEFINED:
          // Nothing to do...
        break;
      }
    break;

    case NMT_JEWEL:
      return PM_JEWEL_106;
    break;

    case NMT_FELICA:
      switch(nm.nbr) {
        case NBR_212:
          return PM_FELICA_212;
        break;
        case NBR_424:
          return PM_FELICA_424;
        break;
        case NBR_106:
        case NBR_847:
        case NBR_UNDEFINED:
          // Nothing to do...
        break;
      }
    break;

    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
    case NMT_DEP:
      // Nothing to do...
    break;
  }
  return PM_UNDEFINED;
}

nfc_modulation_t
pn53x_ptt_to_nm( const pn53x_target_type_t ptt )
{
  switch (ptt) {
    case PTT_GENERIC_PASSIVE_106:
    case PTT_GENERIC_PASSIVE_212:
    case PTT_GENERIC_PASSIVE_424:
    case PTT_UNDEFINED:
      // XXX This should not happend, how handle it cleanly ?
    break;

    case PTT_MIFARE:
    case PTT_ISO14443_4A_106:
      return (const nfc_modulation_t){ .nmt = NMT_ISO14443A, .nbr = NBR_106 };
    break;

    case PTT_ISO14443_4B_106:
    case PTT_ISO14443_4B_TCL_106:
      return (const nfc_modulation_t){ .nmt = NMT_ISO14443B, .nbr = NBR_106 };
    break;

    case PTT_JEWEL_106:
      return (const nfc_modulation_t){ .nmt = NMT_JEWEL, .nbr = NBR_106 };
    break;

    case PTT_FELICA_212:
      return (const nfc_modulation_t){ .nmt = NMT_FELICA, .nbr = NBR_212 };
    break;
    case PTT_FELICA_424:
      return (const nfc_modulation_t){ .nmt = NMT_FELICA, .nbr = NBR_424 };
    break;

    case PTT_DEP_PASSIVE_106:
    case PTT_DEP_ACTIVE_106:
      return (const nfc_modulation_t){ .nmt = NMT_DEP, .nbr = NBR_106 };
    break;
    case PTT_DEP_PASSIVE_212:
    case PTT_DEP_ACTIVE_212:
      return (const nfc_modulation_t){ .nmt = NMT_DEP, .nbr = NBR_212 };
    break;
    case PTT_DEP_PASSIVE_424:
    case PTT_DEP_ACTIVE_424:
      return (const nfc_modulation_t){ .nmt = NMT_DEP, .nbr = NBR_424 };
    break;
  }
  // We should never be here, this line silent compilation warning
  return (const nfc_modulation_t){ .nmt = NMT_ISO14443A, .nbr = NBR_106 };
}

pn53x_target_type_t
pn53x_nm_to_ptt(const nfc_modulation_t nm)
{
  switch(nm.nmt) {
    case NMT_ISO14443A:
      return PTT_MIFARE;
      // return PTT_ISO14443_4A_106;
    break;

    case NMT_ISO14443B:
      switch(nm.nbr) {
        case NBR_106:
          return PTT_ISO14443_4B_106;
        break;
        case NBR_UNDEFINED:
        case NBR_212:
        case NBR_424:
        case NBR_847:
          // Nothing to do...
        break;
      }
    break;

    case NMT_JEWEL:
      return PTT_JEWEL_106;
    break;

    case NMT_FELICA:
      switch(nm.nbr) {
        case NBR_212:
          return PTT_FELICA_212;
        break;
        case NBR_424:
          return PTT_FELICA_424;
        break;
        case NBR_UNDEFINED:
        case NBR_106:
        case NBR_847:
          // Nothing to do...
        break;
      }
    break;

    case NMT_ISO14443BI:
    case NMT_ISO14443B2SR:
    case NMT_ISO14443B2CT:
    case NMT_DEP:
      // Nothing to do...
    break;
  }
  return PTT_UNDEFINED;
}

void
pn53x_data_new (nfc_device_t * pnd, const struct pn53x_io* io)
{
  pnd->chip_data = malloc(sizeof(struct pn53x_data));

  // Keep I/O functions
  CHIP_DATA (pnd)->io = io;

  // Set power mode to normal, if your device starts in LowVBat (ie. PN532
  // UART) the driver layer have to correctly set it.
  CHIP_DATA (pnd)->power_mode = NORMAL;

  // PN53x starts in initiator mode
  CHIP_DATA (pnd)->operating_mode = INITIATOR;

  // Set current target to NULL
  CHIP_DATA (pnd)->current_target = NULL;

  // WriteBack cache is clean
  CHIP_DATA (pnd)->wb_trigged = false;
  memset (CHIP_DATA (pnd)->wb_mask, 0x00, PN53X_CACHE_REGISTER_SIZE);
}

void
pn53x_data_free (nfc_device_t * pnd)
{
  if (CHIP_DATA (pnd)->current_target) {
    free (CHIP_DATA (pnd)->current_target);
  }
  free (pnd->chip_data);
}
