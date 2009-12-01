/**
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
 * 
 * 
 * @file pn53x.h
 * @brief PN531, PN532 and PN533 common functions
 */

#include <string.h>
#include <stdio.h>

#include "pn53x.h"

#include "../bitutils.h"

// PN53X configuration
const byte_t pncmd_get_firmware_version       [  2] = { 0xD4,0x02 };
const byte_t pncmd_get_general_status         [  2] = { 0xD4,0x04 };
const byte_t pncmd_get_register               [  4] = { 0xD4,0x06 };
const byte_t pncmd_set_register               [  5] = { 0xD4,0x08 };
const byte_t pncmd_set_parameters             [  3] = { 0xD4,0x12 };
const byte_t pncmd_rf_configure               [ 14] = { 0xD4,0x32 };

// Reader
const byte_t pncmd_initiator_list_passive        [264] = { 0xD4,0x4A };
const byte_t pncmd_initiator_jump_for_dep        [ 68] = { 0xD4,0x56 };
const byte_t pncmd_initiator_select              [  3] = { 0xD4,0x54 };
const byte_t pncmd_initiator_deselect            [  3] = { 0xD4,0x44,0x00 };
const byte_t pncmd_initiator_release             [  3] = { 0xD4,0x52,0x00 };
const byte_t pncmd_initiator_set_baud_rate       [  5] = { 0xD4,0x4E };
const byte_t pncmd_initiator_exchange_data       [265] = { 0xD4,0x40 };
const byte_t pncmd_initiator_exchange_raw_data   [266] = { 0xD4,0x42 };
const byte_t pncmd_initiator_auto_poll           [  5] = { 0xD4,0x60 };

// Target
const byte_t pncmd_target_get_data            [  2] = { 0xD4,0x86 };
const byte_t pncmd_target_set_data            [264] = { 0xD4,0x8E };
const byte_t pncmd_target_init                [ 39] = { 0xD4,0x8C };
const byte_t pncmd_target_virtual_card        [  4] = { 0xD4,0x14 };
const byte_t pncmd_target_receive             [  2] = { 0xD4,0x88 };
const byte_t pncmd_target_send                [264] = { 0xD4,0x90 };
const byte_t pncmd_target_get_status          [  2] = { 0xD4,0x8A };


bool pn53x_transceive(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  // Check if receiving buffers are available, if not, replace them
  if (!pszRxLen || !pbtRx)
  {
    pbtRx = abtRx;
    pszRxLen = &szRxLen;
  }

  *pszRxLen = MAX_FRAME_LEN;
  // Call the tranceive callback function of the current device
  if (!pnd->pdc->transceive(pnd->nds,pbtTx,szTxLen,pbtRx,pszRxLen)) return false;

  // Make sure there was no failure reported by the PN53X chip (0x00 == OK)
  if (pbtRx[0] != 0) return false;

  // Succesful transmission
  return true;
}

byte_t pn53x_get_reg(const nfc_device_t* pnd, uint16_t ui16Reg)
{
  uint8_t ui8Value;
  size_t szValueLen = 1;
  byte_t abtCmd[sizeof(pncmd_get_register)];
  memcpy(abtCmd,pncmd_get_register,sizeof(pncmd_get_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  pnd->pdc->transceive(pnd->nds,abtCmd,4,&ui8Value,&szValueLen);
  return ui8Value;
}

bool pn53x_set_reg(const nfc_device_t* pnd, uint16_t ui16Reg, uint8_t ui8SybmolMask, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_register)];
  memcpy(abtCmd,pncmd_set_register,sizeof(pncmd_set_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  abtCmd[4] = ui8Value | (pn53x_get_reg(pnd,ui16Reg) & (~ui8SybmolMask));
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  return pnd->pdc->transceive(pnd->nds,abtCmd,5,NULL,NULL);
}

bool pn53x_set_parameters(const nfc_device_t* pnd, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_parameters)];
  memcpy(abtCmd,pncmd_set_parameters,sizeof(pncmd_set_parameters));

  abtCmd[2] = ui8Value;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  return pnd->pdc->transceive(pnd->nds,abtCmd,3,NULL,NULL);
}

bool pn53x_set_tx_bits(const nfc_device_t* pnd, uint8_t ui8Bits)
{
  // Test if we need to update the transmission bits register setting
  if (pnd->ui8TxBits != ui8Bits)
  {
    // Set the amount of transmission bits in the PN53X chip register
    if (!pn53x_set_reg(pnd,REG_CIU_BIT_FRAMING,SYMBOL_TX_LAST_BITS,ui8Bits)) return false;

    // Store the new setting
    ((nfc_device_t*)pnd)->ui8TxBits = ui8Bits;
  }
  return true;
}

bool pn53x_wrap_frame(const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtFrame, size_t* pszFrameBits)
{
  byte_t btFrame;
  byte_t btData;
  uint32_t uiBitPos;
  uint32_t uiDataPos = 0;
  size_t szBitsLeft = szTxBits;

  // Make sure we should frame at least something
  if (szBitsLeft == 0) return false;

  // Handle a short response (1byte) as a special case
  if (szBitsLeft < 9)
  {
    *pbtFrame = *pbtTx;
    *pszFrameBits = szTxBits;
    return true;
  }

  // We start by calculating the frame length in bits
  *pszFrameBits = szTxBits + (szTxBits/8);

  // Parse the data bytes and add the parity bits
  // This is really a sensitive process, mirror the frame bytes and append parity bits
  // buffer = mirror(frame-byte) + parity + mirror(frame-byte) + parity + ...
    // split "buffer" up in segments of 8 bits again and mirror them
  // air-bytes = mirror(buffer-byte) + mirror(buffer-byte) + mirror(buffer-byte) + ..
  while(true)
  {
    // Reset the temporary frame byte;
    btFrame = 0;

    for (uiBitPos=0; uiBitPos<8; uiBitPos++)
    {
      // Copy as much data that fits in the frame byte
      btData = mirror(pbtTx[uiDataPos]);
      btFrame |= (btData >> uiBitPos);
      // Save this frame byte
      *pbtFrame = mirror(btFrame);
      // Set the remaining bits of the date in the new frame byte and append the parity bit
      btFrame = (btData << (8-uiBitPos));
      btFrame |= ((pbtTxPar[uiDataPos] & 0x01) << (7-uiBitPos));
      // Backup the frame bits we have so far
      pbtFrame++;
      *pbtFrame = mirror(btFrame);
      // Increase the data (without parity bit) position
      uiDataPos++;
      // Test if we are done
      if (szBitsLeft < 9) return true;
      szBitsLeft -= 8;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFrame++;
  }
}

bool pn53x_unwrap_frame(const byte_t* pbtFrame, const size_t szFrameBits, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t btFrame;
  byte_t btData;
  uint8_t uiBitPos;
  uint32_t uiDataPos = 0;
  byte_t* pbtFramePos = (byte_t*) pbtFrame;
  size_t szBitsLeft = szFrameBits;

  // Make sure we should frame at least something
  if (szBitsLeft == 0) return false;

  // Handle a short response (1byte) as a special case
  if (szBitsLeft < 9)
  {
    *pbtRx = *pbtFrame;
    *pszRxBits = szFrameBits;
    return true;
  }

  // Calculate the data length in bits
  *pszRxBits = szFrameBits - (szFrameBits/9);

  // Parse the frame bytes, remove the parity bits and store them in the parity array
  // This process is the reverse of WrapFrame(), look there for more info
  while(true)
  {
    for (uiBitPos=0; uiBitPos<8; uiBitPos++)
    {
      btFrame = mirror(pbtFramePos[uiDataPos]);
      btData = (btFrame << uiBitPos);
      btFrame = mirror(pbtFramePos[uiDataPos+1]);
      btData |= (btFrame >> (8-uiBitPos));
      pbtRx[uiDataPos] = mirror(btData);
      if(pbtRxPar != NULL) pbtRxPar[uiDataPos] = ((btFrame >> (7-uiBitPos)) & 0x01);
      // Increase the data (without parity bit) position
      uiDataPos++;
      // Test if we are done
      if (szBitsLeft < 9) return true;
      szBitsLeft -= 9;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFramePos++;
  }
}

