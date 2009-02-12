/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <string.h>
#include "libnfc.h"
#include "acr122.h"
#include "bitutils.h"

#define REG_CIU_TX_MODE       0x6302
#define REG_CIU_RX_MODE       0x6303
#define REG_CIU_TX_AUTO       0x6305
#define REG_CIU_MANUAL_RCV    0x630D
#define REG_CIU_CONTROL       0x633C
#define REG_CIU_BIT_FRAMING   0x633D

#define PARAM_NONE            0x00
#define PARAM_NAD_USED        0x01
#define PARAM_DID_USED        0x02
#define PARAM_AUTO_ATR_RES    0x04
#define PARAM_AUTO_RATS       0x10
#define PARAM_14443_4_PICC    0x20
#define PARAM_NO_AMBLE        0x40

// PN532 configuration
byte pncmd_get_firmware_version       [  2] = { 0xD4,0x02 };
byte pncmd_get_general_status         [  2] = { 0xD4,0x04 };
byte pncmd_get_register               [  4] = { 0xD4,0x06 };
byte pncmd_set_register               [  5] = { 0xD4,0x08 };
byte pncmd_set_parameters             [  3] = { 0xD4,0x12 };

// RF field configuration
byte pncmd_rf_configure_field         [  4] = { 0xD4,0x32,0x01 };
byte pncmd_rf_configure_timing        [  4] = { 0xD4,0x32,0x02 };
byte pncmd_rf_configure_retry_data    [  4] = { 0xD4,0x32,0x04 };
byte pncmd_rf_configure_retry_select  [  6] = { 0xD4,0x32,0x05 };

// Reader
byte pncmd_reader_list_passive        [264] = { 0xD4,0x4A };
byte pncmd_reader_select              [  3] = { 0xD4,0x54 };
byte pncmd_reader_deselect            [  3] = { 0xD4,0x44 };
byte pncmd_reader_release             [  3] = { 0xD4,0x52 };
byte pncmd_reader_set_baud_rate       [  5] = { 0xD4,0x4E };
byte pncmd_reader_exchange_data       [265] = { 0xD4,0x40 };
byte pncmd_reader_auto_poll           [  5] = { 0xD4,0x60 };

// Target
byte pncmd_target_get_data            [  2] = { 0xD4,0x86 };
byte pncmd_target_init                [ 39] = { 0xD4,0x8C };
byte pncmd_target_virtual_card        [  4] = { 0xD4,0x14 };
byte pncmd_target_receive             [  2] = { 0xD4,0x88 };
byte pncmd_target_send                [264] = { 0xD4,0x90 };
byte pncmd_target_get_status          [  2] = { 0xD4,0x8A };

// Exchange raw data frames
byte pncmd_exchange_raw_data          [266] = { 0xD4,0x42 };

// Global buffers used for communication with the PN532 chip
#define MAX_FRAME_LEN 264
static byte abtRx[MAX_FRAME_LEN];
static ui32 uiRxLen;
static ui8 ui8Value;
static ui32 ui32Len = 1;

bool pn532_transceive(const dev_id di, const byte* pbtTx, const ui32 uiTxLen)
{
  uiRxLen = MAX_FRAME_LEN;
  return acr122_transceive(di,pbtTx,uiTxLen,abtRx,&uiRxLen);
}

bool pn532_set_reg(const dev_id di, ui16 ui16Reg, ui8 ui8Value)
{
  pncmd_set_register[2] = ui16Reg >> 8;
  pncmd_set_register[3] = ui16Reg & 0xff;
  pncmd_set_register[4] = ui8Value;
  return acr122_transceive(di,pncmd_set_register,5,null,null);
}

byte pn532_get_reg(const dev_id di, ui16 ui16Reg)
{
  pncmd_get_register[2] = ui16Reg >> 8;
  pncmd_get_register[3] = ui16Reg & 0xff;
  acr122_transceive(di,pncmd_get_register,4,&ui8Value,&ui32Len);
  return ui8Value;
}

bool pn532_set_parameters(const dev_id di, ui8 ui8Value)
{
  pncmd_set_parameters[2] = ui8Value;
  return acr122_transceive(di,pncmd_set_parameters,3,null,null);
}

bool pn532_wrap_frame(const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar, byte* pbtFrame, ui32* puiFrameBits)
{
  byte btFrame;
  byte btData;
  ui32 uiBitPos;
  ui32 uiDataPos = 0;
  ui32 uiBitsLeft = uiTxBits;

  // Make sure we should frame at least something
  if (uiBitsLeft == 0) return false;
  
  // Handle a short response (1byte) as a special case
  if (uiBitsLeft < 9)
  {
    *pbtFrame = *pbtTx;
    *puiFrameBits = uiTxBits;
    return true;
  }

  // We start by calculating the frame length in bits
  *puiFrameBits = uiTxBits + (uiTxBits/8);
    
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
      if (uiBitsLeft < 9) return true;
      uiBitsLeft -= 8;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFrame++;
  }
}

bool pn532_unwrap_frame(const byte* pbtFrame, const ui32 uiFrameBits, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar)
{
  byte btFrame;
  byte btData;
  ui8 uiBitPos;
  ui32 uiDataPos = 0;
  byte* pbtFramePos = (byte*) pbtFrame;
  ui32 uiBitsLeft = uiFrameBits;

  // Make sure we should frame at least something
  if (uiBitsLeft == 0) return false;

  // Handle a short response (1byte) as a special case
  if (uiBitsLeft < 9)
  {
    *pbtRx = *pbtFrame;
    *puiRxBits = uiFrameBits;
    return true;
  }
  
  // Calculate the data length in bits
  *puiRxBits = uiFrameBits - (uiFrameBits/9);

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
      if(pbtRxPar != null) pbtRxPar[uiDataPos] = ((btFrame >> (7-uiBitPos)) & 0x01);
      // Increase the data (without parity bit) position
      uiDataPos++;
      // Test if we are done
      if (uiBitsLeft < 9) return true;
      uiBitsLeft -= 9;
    }
    // Every 8 data bytes we lose one frame byte to the parities
    pbtFramePos++;
  }
}

bool nfc_configure_handle_crc(const dev_id di, const bool bEnable)
{
  if (bEnable)
  {
    // Enable automatic receiving/sending of CRC bytes
    if (!pn532_set_reg(di,REG_CIU_TX_MODE,pn532_get_reg(di,REG_CIU_TX_MODE) | 0x80)) return false;
	  if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) | 0x80)) return false;
  } else {
    // Disable automatic receiving/sending of CRC bytes
    if (!pn532_set_reg(di,REG_CIU_TX_MODE,pn532_get_reg(di,REG_CIU_TX_MODE) & 0x7f)) return false;
	  if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) & 0x7f)) return false;
  }
  return true;
}

bool nfc_configure_handle_parity(const dev_id di, const bool bEnable)
{
  if (bEnable)
  {
    // Handle parity by chip
    if (!pn532_set_reg(di,REG_CIU_MANUAL_RCV,pn532_get_reg(di,REG_CIU_MANUAL_RCV) & 0xef)) return false;

  } else {
    // Parse parity as data bit
    if (!pn532_set_reg(di,REG_CIU_MANUAL_RCV,pn532_get_reg(di,REG_CIU_MANUAL_RCV) | 0x10)) return false;
  }
  return true;
}

bool nfc_configure_field(const dev_id di, const bool bEnable)
{
  pncmd_rf_configure_field[3] = (bEnable) ? 1 : 0;
  return acr122_transceive(di,pncmd_rf_configure_field,4,null,null);
}

bool nfc_configure_list_passive_infinite(const dev_id di, const bool bEnable)
{
  // Retry format: 0x00 means only 1 try, 0xff means infinite
  pncmd_rf_configure_retry_select[3] = (bEnable) ? 0xff : 0x00; // MxRtyATR, default: active = 0xff, passive = 0x02
  pncmd_rf_configure_retry_select[4] = (bEnable) ? 0xff : 0x00; // MxRtyPSL, default: 0x01
  pncmd_rf_configure_retry_select[5] = (bEnable) ? 0xff : 0x00; // MxRtyPassiveActivation, default: 0xff
  return acr122_transceive(di,pncmd_rf_configure_retry_select,6,null,null);
}

bool nfc_configure_accept_invalid_frames(const dev_id di, const bool bEnable)
{
  if (bEnable)
  {
    // Handle parity by chip
    if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) | 0x08)) return false;

  } else {
    // Parse parity as data bit
    if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) | 0xf7)) return false;
  }
  return true;
}

bool nfc_configure_accept_multiple_frames(const dev_id di, const bool bEnable)
{
  if (bEnable)
  {
    // Handle parity by chip
    if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) | 0x04)) return false;

  } else {
    // Parse parity as data bit
    if (!pn532_set_reg(di,REG_CIU_RX_MODE,pn532_get_reg(di,REG_CIU_RX_MODE) | 0xfb)) return false;
  }
  return true;
}

bool nfc_reader_init(const dev_id di)
{
	// Try to connect to the NFC reader
  if (di == INVALID_DEVICE_ID) return INVALID_DEVICE_ID;
  
  // Let the PN5XX automatically be activated by the RF level detector
  if (!pn532_set_reg(di,REG_CIU_TX_AUTO,pn532_get_reg(di,REG_CIU_TX_AUTO) | 0x40)) return false;

  // Configure the PN532 to be an Initiator or Reader/Writer
  if (!pn532_set_reg(di,REG_CIU_CONTROL,pn532_get_reg(di,REG_CIU_CONTROL) | 0x10)) return false;

  return true;
}

bool nfc_reader_list_passive(const dev_id di, const ModulationType mt, const byte* pbtInitData, const ui32 uiInitDataLen, byte* pbtTag, ui32* puiTagLen)
{
  pncmd_reader_list_passive[2] = 1; // MaxTg, we only want to select 1 tag at the time
  pncmd_reader_list_passive[3] = mt; // BrTy, the type of modulation used for polling a passive tag

  // Set the optional initiator data (used for Felica, ISO14443B, Topaz Polling or selecting a specific UID).
  if (pbtInitData) memcpy(pncmd_reader_list_passive+4,pbtInitData,uiInitDataLen);

  // Try to find the available tags
  if (!acr122_transceive(di,pncmd_reader_list_passive,4+uiInitDataLen,pbtTag,puiTagLen)) return false;
  
  // Return true only if at least one tag has been found, the PN532 returns 0x00 if none was available
  return (pbtTag[0] != 0x00);
}

bool nfc_reader_transceive_7bits(const dev_id di, const byte btTx, byte* pbtRx, ui32* puiRxLen)
{
  // Set transmission bits to 7
  if (!pn532_set_reg(di,REG_CIU_BIT_FRAMING,7)) return false;

  pncmd_exchange_raw_data[2] = btTx;
  if (!pn532_transceive(di,pncmd_exchange_raw_data,3)) return false;
  
  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  *puiRxLen = uiRxLen-1;
  memcpy(pbtRx,abtRx+1,*puiRxLen);

  // Reset transmission bits count
  if (!pn532_set_reg(di,REG_CIU_BIT_FRAMING,0)) return false;

  return true;
}

bool nfc_reader_transceive_bytes(const dev_id di, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen)
{
  memcpy(pncmd_exchange_raw_data+2,pbtTx,uiTxLen);
  
  if (!pn532_transceive(di,pncmd_exchange_raw_data,2+uiTxLen)) return false;
  
  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  *puiRxLen = uiRxLen-1;
  memcpy(pbtRx,abtRx+1,*puiRxLen);
  
  return true;
}

bool nfc_reader_transceive_bits(const dev_id di, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar)
{
  ui32 uiFrameBits = 0;
  ui8 ui8Bits = 0;

  // Convert data to a frame
  pn532_wrap_frame(pbtTx,uiTxBits,pbtTxPar,pncmd_exchange_raw_data+2,&uiFrameBits);

  // Retrieve the resting bits
  ui8Bits = uiFrameBits%8;
  
  // Set the amount of transmission bits in the pn532 chip register
  if (!pn532_set_reg(di,REG_CIU_BIT_FRAMING,ui8Bits)) return false;
  
  // Send the frame to the pn532 chip and get the answer
  // We have to give the amount of bytes + 1 byte if there are leading bits + the two 0xD4,0x42
  if (!pn532_transceive(di,pncmd_exchange_raw_data,(uiFrameBits/8)+((ui8Bits==0)?0:1)+2)) return false;
 
  // Make sure there was no failure reported by the PN532 chip (0 == OK)
  // TODO: is this ACR122 specific?
  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  
  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn532_get_reg(di,REG_CIU_CONTROL) & 0x07;

  // Unwrap the response frame
  pn532_unwrap_frame(abtRx+1,((uiRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits,pbtRx,puiRxBits,pbtRxPar);

  // Everything went successful
  return true;
}

bool nfc_reader_mifare_cmd(const dev_id di, const MifareCmd mc, const ui8 ui8Block, MifareParam* pmp)
{
  ui32 uiParamLen;
  pncmd_reader_exchange_data[2] = 0x01;     // Use first target/card
  pncmd_reader_exchange_data[3] = mc;       // The MIFARE Classic command
  pncmd_reader_exchange_data[4] = ui8Block; // The block address (1K=0x00..0x39, 4K=0x00..0xff)
  
  switch (mc)
  {
    // Read and store command have no parameter
    case MC_READ:
    case MC_STORE:
      uiParamLen = 0;
    break;
   
    // Authenticate command
    case MC_AUTH_A:
    case MC_AUTH_B:
      uiParamLen = sizeof(MifareParamAuth);
    break;

    // Data command
    case MC_WRITE:
      uiParamLen = sizeof(MifareParamData);
    break;

    // Value command
    case MC_DECREMENT:
    case MC_INCREMENT:
    case MC_TRANSFER:
      uiParamLen = sizeof(MifareParamValue);
    break;

    // Please fix your code, you never should reach this statement
    default:
      return false;
    break;
  }
  
  // When available, copy the parameter bytes
  if (uiParamLen) memcpy(pncmd_reader_exchange_data+5,(byte*)pmp,uiParamLen);
  
  // Fire the mifare command
  if (!pn532_transceive(di,pncmd_reader_exchange_data,5+uiParamLen)) return false;

  // Make sure there was no failure reported by the PN532 chip (0 == OK)
  // TODO: is this ACR122 specific?
  if (abtRx[0] != 0) return false;

  // When we have executed a read command, copy the received bytes into the param
  if (mc == MC_READ) memcpy(pmp->mpd.abtData,abtRx+1,16);

  // Command succesfully executed
  return true;
}

bool nfc_target_init(const dev_id di, byte* pbtRx, ui32* puiRxLen)
{
	// Clear the target init struct, reset to all zeros
  memset(pncmd_target_init+2,0x00,37);

	// Set ATQA (SENS_RES)
	pncmd_target_init[3] = 0x04;
	pncmd_target_init[4] = 0x00;

	// Set SAK (SEL_RES)
  pncmd_target_init[8] = 0x20;

	// Set UID
	pncmd_target_init[5] = 0x00;
	pncmd_target_init[6] = 0xb0;
	pncmd_target_init[7] = 0x0b;
	
  // Enable CRC & parity, needed for target_init to work properly
  nfc_configure_handle_crc(di,true);
  nfc_configure_handle_parity(di,true);

  // Request the initialization as a target
  if (!pn532_transceive(di,pncmd_target_init,39)) return false;

  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  *puiRxLen = uiRxLen-1;
  memcpy(pbtRx,abtRx+1,*puiRxLen);

  return true;
}

bool nfc_target_receive_bytes(const dev_id di, byte* pbtRx, ui32* puiRxLen)
{
  if (!pn532_transceive(di,pncmd_target_receive,2)) return false;
  
  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  *puiRxLen = uiRxLen-1;
  memcpy(pbtRx,abtRx+1,*puiRxLen);
  
  return true;
}

bool nfc_target_receive_bits(const dev_id di, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar)
{
	ui8 ui8Bits;

	// Try to gather the received frames from the reader
	if (!pn532_transceive(di,pncmd_target_receive,2)) return false;

  // Make sure there was no failure reported by the PN532 chip (0 == OK)
  // TODO: is this ACR122 specific?
  if (uiRxLen == 1 || abtRx[0] != 0) return false;
  
  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn532_get_reg(di,REG_CIU_CONTROL) & 0x07;

  // Unwrap the response frame
  pn532_unwrap_frame(abtRx+1,((uiRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits,pbtRx,puiRxBits,pbtRxPar);

	return true;
}

bool nfc_target_send_bytes(const dev_id di, const byte* pbtTx, const ui32 uiTxLen)
{
  memcpy(pncmd_target_send+2,pbtTx,uiTxLen);
  return acr122_transceive(di,pncmd_target_send,2+uiTxLen,null,null);
}

bool nfc_target_send_bits(const dev_id di, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar)
{
  ui32 uiFrameBits = 0;
  ui8 ui8Bits = 0;

  // Convert data to a frame
  pn532_wrap_frame(pbtTx,uiTxBits,pbtTxPar,pncmd_target_send+2,&uiFrameBits);

  // Retrieve the resting bits
  ui8Bits = uiFrameBits%8;
  
  // Set the amount of transmission bits in the pn532 chip register
  if (!pn532_set_reg(di,REG_CIU_BIT_FRAMING,ui8Bits)) return false;

  // Try to send the bits to the reader
  if (!pn532_transceive(di,pncmd_target_send,(uiFrameBits/8)+((ui8Bits==0)?0:1)+2)) return false;

  // Make sure there was no failure reported by the PN532 chip (0 == OK)
  // TODO: is this ACR122 specific?
  if (uiRxLen == 1 || abtRx[0] != 0) return false;

  // Everyting seems ok, return true
  return true;
}

