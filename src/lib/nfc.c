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
 * @file nfc.c
 * @brief
 */

#include "nfc.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "drivers.h"

#include "messages.h"

#include "../../config.h"

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

bool pn53x_transceive(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
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
  if (!pdi->pdc->transceive(pdi->ds,pbtTx,szTxLen,pbtRx,pszRxLen)) return false;

  // Make sure there was no failure reported by the PN53X chip (0x00 == OK)
  if (pbtRx[0] != 0) return false;

  // Succesful transmission
  return true;
}

byte_t pn53x_get_reg(const dev_info* pdi, uint16_t ui16Reg)
{
  uint8_t ui8Value;
  size_t szValueLen = 1;
  byte_t abtCmd[sizeof(pncmd_get_register)];
  memcpy(abtCmd,pncmd_get_register,sizeof(pncmd_get_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  pdi->pdc->transceive(pdi->ds,abtCmd,4,&ui8Value,&szValueLen);
  return ui8Value;
}

bool pn53x_set_reg(const dev_info* pdi, uint16_t ui16Reg, uint8_t ui8SybmolMask, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_register)];
  memcpy(abtCmd,pncmd_set_register,sizeof(pncmd_set_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  abtCmd[4] = ui8Value | (pn53x_get_reg(pdi,ui16Reg) & (~ui8SybmolMask));
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  return pdi->pdc->transceive(pdi->ds,abtCmd,5,NULL,NULL);
}

bool pn53x_set_parameters(const dev_info* pdi, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_parameters)];
  memcpy(abtCmd,pncmd_set_parameters,sizeof(pncmd_set_parameters));

  abtCmd[2] = ui8Value;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  return pdi->pdc->transceive(pdi->ds,abtCmd,3,NULL,NULL);
}

bool pn53x_set_tx_bits(const dev_info* pdi, uint8_t ui8Bits)
{
  // Test if we need to update the transmission bits register setting
  if (pdi->ui8TxBits != ui8Bits)
  {
    // Set the amount of transmission bits in the PN53X chip register
    if (!pn53x_set_reg(pdi,REG_CIU_BIT_FRAMING,SYMBOL_TX_LAST_BITS,ui8Bits)) return false;

    // Store the new setting
    ((dev_info*)pdi)->ui8TxBits = ui8Bits;
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

dev_info* nfc_connect(nfc_device_desc_t* pndd)
{
  dev_info* pdi;
  uint32_t uiDev;
  byte_t abtFw[4];
  size_t szFwLen = sizeof(abtFw);

  // Search through the device list for an available device
  for (uiDev=0; uiDev<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDev++)
  {
    if (pndd == NULL) {
      // No device description specified: try to automatically claim a device
      pdi = drivers_callbacks_list[uiDev].connect(pndd);
    } else {
      // Specific device is requested: using device description pndd
      if( 0 != strcmp(drivers_callbacks_list[uiDev].acDriver, pndd->pcDriver ) )
      {
        DBG("Looking for %s, found %s... Skip it.", pndd->pcDriver, drivers_callbacks_list[uiDev].acDriver);
        continue;
      } else {
        DBG("Looking for %s, found %s... Use it.", pndd->pcDriver, drivers_callbacks_list[uiDev].acDriver);
        pdi = drivers_callbacks_list[uiDev].connect(pndd);
      }
    }

    // Test if the connection was successful
    if (pdi != INVALID_DEVICE_INFO)
    {
      DBG("[%s] has been claimed.", pdi->acName);
      // Great we have claimed a device
      pdi->pdc = &(drivers_callbacks_list[uiDev]);

      // Try to retrieve PN53x chip revision
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pdi->pdc->transceive(pdi->ds,pncmd_get_firmware_version,2,abtFw,&szFwLen))
      {
        // Failed to get firmware revision??, whatever...let's disconnect and clean up and return err
        ERR("Failed to get firmware revision for: %s", pdi->acName);
        pdi->pdc->disconnect(pdi);
        return INVALID_DEVICE_INFO;
      }

      // Add the firmware revision to the device name, PN531 gives 2 bytes info, but PN532 gives 4
      switch(pdi->ct)
      {
        case CT_PN531: sprintf(pdi->acName,"%s - PN531 v%d.%d",pdi->acName,abtFw[0],abtFw[1]); break;
        case CT_PN532: sprintf(pdi->acName,"%s - PN532 v%d.%d (0x%02x)",pdi->acName,abtFw[1],abtFw[2],abtFw[3]); break;
        case CT_PN533: sprintf(pdi->acName,"%s - PN533 v%d.%d (0x%02x)",pdi->acName,abtFw[1],abtFw[2],abtFw[3]); break;
      }

      // Reset the ending transmission bits register, it is unknown what the last tranmission used there
      if (!pn53x_set_reg(pdi,REG_CIU_BIT_FRAMING,SYMBOL_TX_LAST_BITS,0x00)) return INVALID_DEVICE_INFO;

      // Make sure we reset the CRC and parity to chip handling.
      if (!nfc_configure(pdi,DCO_HANDLE_CRC,true)) return INVALID_DEVICE_INFO;
      if (!nfc_configure(pdi,DCO_HANDLE_PARITY,true)) return INVALID_DEVICE_INFO;

      // Deactivate the CRYPTO1 chiper, it may could cause problems when still active
      if (!nfc_configure(pdi,DCO_ACTIVATE_CRYPTO1,false)) return INVALID_DEVICE_INFO;

      return pdi;
    } else {
      DBG("No device found using driver: %s", drivers_callbacks_list[uiDev].acDriver);
    }
  }
  // To bad, no reader is ready to be claimed
  return INVALID_DEVICE_INFO;
}

void nfc_disconnect(dev_info* pdi)
{
  // Release and deselect all active communications
  nfc_initiator_deselect_tag(pdi);
  // Disconnect, clean up and release the device 
  pdi->pdc->disconnect(pdi);
}

bool nfc_configure(dev_info* pdi, const dev_config_option dco, const bool bEnable)
{
  byte_t btValue;
  byte_t abtCmd[sizeof(pncmd_rf_configure)];
  memcpy(abtCmd,pncmd_rf_configure,sizeof(pncmd_rf_configure));

  // Make sure we are dealing with a active device
  if (!pdi->bActive) return false;

  switch(dco)
  {
    case DCO_HANDLE_CRC:
      // Enable or disable automatic receiving/sending of CRC bytes
      // TX and RX are both represented by the symbol 0x80
      btValue = (bEnable) ? 0x80 : 0x00;
      if (!pn53x_set_reg(pdi,REG_CIU_TX_MODE,SYMBOL_TX_CRC_ENABLE,btValue)) return false;
      if (!pn53x_set_reg(pdi,REG_CIU_RX_MODE,SYMBOL_RX_CRC_ENABLE,btValue)) return false;
      pdi->bCrc = bEnable;
    break;

    case DCO_HANDLE_PARITY:
      // Handle parity bit by PN53X chip or parse it as data bit
      btValue = (bEnable) ? 0x00 : SYMBOL_PARITY_DISABLE;
      if (!pn53x_set_reg(pdi,REG_CIU_MANUAL_RCV,SYMBOL_PARITY_DISABLE,btValue)) return false;
      pdi->bPar = bEnable;
    break;

    case DCO_ACTIVATE_FIELD:
      abtCmd[2] = RFCI_FIELD;
      abtCmd[3] = (bEnable) ? 1 : 0;
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pdi->pdc->transceive(pdi->ds,abtCmd,4,NULL,NULL)) return false;
    break;

    case DCO_ACTIVATE_CRYPTO1:
      btValue = (bEnable) ? SYMBOL_MF_CRYPTO1_ON : 0x00;
      if (!pn53x_set_reg(pdi,REG_CIU_STATUS2,SYMBOL_MF_CRYPTO1_ON,btValue)) return false;
    break;

    case DCO_INFINITE_SELECT:
      // Retry format: 0x00 means only 1 try, 0xff means infinite
      abtCmd[2] = RFCI_RETRY_SELECT;
      abtCmd[3] = (bEnable) ? 0xff : 0x00; // MxRtyATR, default: active = 0xff, passive = 0x02
      abtCmd[4] = (bEnable) ? 0xff : 0x00; // MxRtyPSL, default: 0x01
      abtCmd[5] = (bEnable) ? 0xff : 0x00; // MxRtyPassiveActivation, default: 0xff
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pdi->pdc->transceive(pdi->ds,abtCmd,6,NULL,NULL)) return false;
    break;

    case DCO_ACCEPT_INVALID_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_NO_ERROR : 0x00;
      if (!pn53x_set_reg(pdi,REG_CIU_RX_MODE,SYMBOL_RX_NO_ERROR,btValue)) return false;
    break;

    case DCO_ACCEPT_MULTIPLE_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_MULTIPLE : 0x00;
      if (!pn53x_set_reg(pdi,REG_CIU_RX_MODE,SYMBOL_RX_MULTIPLE,btValue)) return false;
    return true;

    break;
  }

  // When we reach this, the configuration is completed and succesful
  return true;
}

bool nfc_initiator_init(const dev_info* pdi)
{
  // Make sure we are dealing with a active device
  if (!pdi->bActive) return false;

  // Set the PN53X to force 100% ASK Modified miller decoding (default for 14443A cards)
  if (!pn53x_set_reg(pdi,REG_CIU_TX_AUTO,SYMBOL_FORCE_100_ASK,0x40)) return false;

  // Configure the PN53X to be an Initiator or Reader/Writer
  if (!pn53x_set_reg(pdi,REG_CIU_CONTROL,SYMBOL_INITIATOR,0x10)) return false;

  return true;
}

bool nfc_initiator_select_dep_target(const dev_info* pdi, const init_modulation im, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, tag_info* pti)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t offset;
  byte_t abtCmd[sizeof(pncmd_initiator_jump_for_dep)];
  memcpy(abtCmd,pncmd_initiator_jump_for_dep,sizeof(pncmd_initiator_jump_for_dep));

  if(im == IM_ACTIVE_DEP) {
    abtCmd[2] = 0x01; /* active DEP */
  }
  abtCmd[3] = 0x00; /* baud rate = 106kbps */

  offset = 5;
  if(pbtPidData && im != IM_ACTIVE_DEP) { /* can't have passive initiator data when using active mode */
    abtCmd[4] |= 0x01;
    memcpy(abtCmd+offset,pbtPidData,szPidDataLen);
    offset+= szPidDataLen;
  }

  if(pbtNFCID3i) {
    abtCmd[4] |= 0x02;
    memcpy(abtCmd+offset,pbtNFCID3i,szNFCID3iDataLen);
    offset+= szNFCID3iDataLen;
  }

  if(pbtGbData) {
    abtCmd[4] |= 0x04;
    memcpy(abtCmd+offset,pbtGbData,szGbDataLen);
    offset+= szGbDataLen;
  }

  // Try to find a target, call the transceive callback function of the current device
  if (!pn53x_transceive(pdi,abtCmd,5+szPidDataLen+szNFCID3iDataLen+szGbDataLen,abtRx,&szRxLen)) return false;

  // Make sure one target has been found, the PN53X returns 0x00 if none was available
  if (abtRx[1] != 1) return false;

  // Is a target info struct available
  if (pti)
  {
    memcpy(pti->tid.NFCID3i,abtRx+2,10);
    pti->tid.btDID = abtRx[12];
    pti->tid.btBSt = abtRx[13];
    pti->tid.btBRt = abtRx[14];
  }
  return true;
}

bool nfc_initiator_select_tag(const dev_info* pdi, const init_modulation im, const byte_t* pbtInitData, const size_t szInitDataLen, tag_info* pti)
{
  byte_t abtInit[MAX_FRAME_LEN];
  size_t szInitLen;
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_list_passive)];
  memcpy(abtCmd,pncmd_initiator_list_passive,sizeof(pncmd_initiator_list_passive));

  // Make sure we are dealing with a active device
  if (!pdi->bActive) return false;

  abtCmd[2] = 1;  // MaxTg, we only want to select 1 tag at the time
  abtCmd[3] = im; // BrTy, the type of init modulation used for polling a passive tag

  switch(im)
  {
    case IM_ISO14443A_106:
      switch (szInitDataLen)
      {
        case 7:
          abtInit[0] = 0x88;
          memcpy(abtInit+1,pbtInitData,7);
          szInitLen = 8;
        break;

        case 10:
          abtInit[0] = 0x88;
          memcpy(abtInit+1,pbtInitData,3);
          abtInit[4] = 0x88;
          memcpy(abtInit+4,pbtInitData+3,7);
          szInitLen = 12;
        break;

        case 4:
        default:
          memcpy(abtInit,pbtInitData,szInitDataLen);
          szInitLen = szInitDataLen;
        break;
      }
    break;

    default:
      memcpy(abtInit,pbtInitData,szInitDataLen);
      szInitLen = szInitDataLen;
    break;
  }

  // Set the optional initiator data (used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID).
  if (pbtInitData) memcpy(abtCmd+4,abtInit,szInitLen);

  // Try to find a tag, call the tranceive callback function of the current device
  szRxLen = MAX_FRAME_LEN;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  if (!pdi->pdc->transceive(pdi->ds,abtCmd,4+szInitLen,abtRx,&szRxLen)) return false;

  // Make sure one tag has been found, the PN53X returns 0x00 if none was available
  if (abtRx[0] != 1) return false;

  // Is a tag info struct available
  if (pti)
  {
    // Fill the tag info struct with the values corresponding to this init modulation
    switch(im)
    {
      case IM_ISO14443A_106:
        // Somehow they switched the lower and upper ATQA bytes around for the PN531 chipset
        if (pdi->ct == CT_PN531)
        {
          pti->tia.abtAtqa[0] = abtRx[3];
          pti->tia.abtAtqa[1] = abtRx[2];
        } else {
          memcpy(pti->tia.abtAtqa,abtRx+2,2);
        }
        pti->tia.btSak = abtRx[4];
        // Copy the NFCID1
        pti->tia.szUidLen = abtRx[5];
        memcpy(pti->tia.abtUid,abtRx+6,pti->tia.szUidLen);
        // Did we received an optional ATS (Smardcard ATR)
        if (szRxLen > pti->tia.szUidLen+6)
        {
          pti->tia.szAtsLen = abtRx[pti->tia.szUidLen+6];
          memcpy(pti->tia.abtAts,abtRx+pti->tia.szUidLen+6,pti->tia.szAtsLen);
        } else {
          pti->tia.szAtsLen = 0;
        }
      break;

      case IM_FELICA_212:
      case IM_FELICA_424:
        // Store the mandatory info
        pti->tif.szLen = abtRx[2];
        pti->tif.btResCode = abtRx[3];
        // Copy the NFCID2t
        memcpy(pti->tif.abtId,abtRx+4,8);
        // Copy the felica padding
        memcpy(pti->tif.abtPad,abtRx+12,8);
        // Test if the System code (SYST_CODE) is available
        if (szRxLen > 20)
        {
          memcpy(pti->tif.abtSysCode,abtRx+20,2);  
        }
      break;

      case IM_ISO14443B_106:
        // Store the mandatory info
        memcpy(pti->tib.abtAtqb,abtRx+2,12);
        // Ignore the 0x1D byte, and just store the 4 byte id
        memcpy(pti->tib.abtId,abtRx+15,4);
        pti->tib.btParam1 = abtRx[19];
        pti->tib.btParam2 = abtRx[20];
        pti->tib.btParam3 = abtRx[21];
        pti->tib.btParam4 = abtRx[22];
        // Test if the Higher layer (INF) is available
        if (szRxLen > 22)
        {
          pti->tib.szInfLen = abtRx[23];
          memcpy(pti->tib.abtInf,abtRx+24,pti->tib.szInfLen);
        } else {
          pti->tib.szInfLen = 0;
        }
      break;

      case IM_JEWEL_106:
        // Store the mandatory info
        memcpy(pti->tij.btSensRes,abtRx+2,2);
        memcpy(pti->tij.btId,abtRx+4,4);
      break;

      default:
        // Should not be possible, so whatever...
      break;
    }
  }
  return true;
}

bool nfc_initiator_deselect_tag(const dev_info* pdi)
{
  return (pn53x_transceive(pdi,pncmd_initiator_deselect,3,NULL,NULL));
}

bool nfc_initiator_transceive_bits(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szFrameBits = 0;
  size_t szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_raw_data,sizeof(pncmd_initiator_exchange_raw_data));

  // Check if we should prepare the parity bits ourself
  if (!pdi->bPar)
  {
    // Convert data with parity to a frame
    pn53x_wrap_frame(pbtTx,szTxBits,pbtTxPar,abtCmd+2,&szFrameBits);
  } else {
    szFrameBits = szTxBits;
  }

  // Retrieve the leading bits
  ui8Bits = szFrameBits%8;

  // Get the amount of frame bytes + optional (1 byte if there are leading bits) 
  szFrameBytes = (szFrameBits/8)+((ui8Bits==0)?0:1);

  // When the parity is handled before us, we just copy the data
  if (pdi->bPar) memcpy(abtCmd+2,pbtTx,szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits(pdi,ui8Bits)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pdi,abtCmd,szFrameBytes+2,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pdi,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  szFrameBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;

  // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
  // Check if we should recover the parity bits ourself
  if (!pdi->bPar)
  {
    // Unwrap the response frame
    pn53x_unwrap_frame(abtRx+1,szFrameBits,pbtRx,pszRxBits,pbtRxPar);
  } else {
    // Save the received bits
    *pszRxBits = szFrameBits;
    // Copy the received bytes
    memcpy(pbtRx,abtRx+1,szRxLen-1);
  }

  // Everything went successful
  return true;
}

bool nfc_initiator_transceive_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_data,sizeof(pncmd_initiator_exchange_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pdi->bPar) return false;

  // Copy the data into the command frame
  abtCmd[2] = 1; /* target number */
  memcpy(abtCmd+3,pbtTx,szTxLen);

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits(pdi,0)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pdi,abtCmd,szTxLen+3,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everything went successful
  return true;
}

bool nfc_initiator_transceive_bytes(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_raw_data,sizeof(pncmd_initiator_exchange_raw_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pdi->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits(pdi,0)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pdi,abtCmd,szTxLen+2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everything went successful
  return true;
}

bool nfc_initiator_mifare_cmd(const dev_info* pdi, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szParamLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_data,sizeof(pncmd_initiator_exchange_data));

  // Make sure we are dealing with a active device
  if (!pdi->bActive) return false;

  abtCmd[2] = 0x01;     // Use first target/card
  abtCmd[3] = mc;       // The MIFARE Classic command
  abtCmd[4] = ui8Block; // The block address (1K=0x00..0x39, 4K=0x00..0xff)

  switch (mc)
  {
    // Read and store command have no parameter
    case MC_READ:
    case MC_STORE:
      szParamLen = 0;
    break;

    // Authenticate command
    case MC_AUTH_A:
    case MC_AUTH_B:
      szParamLen = sizeof(mifare_param_auth);
    break;

    // Data command
    case MC_WRITE:
      szParamLen = sizeof(mifare_param_data);
    break;

    // Value command
    case MC_DECREMENT:
    case MC_INCREMENT:
    case MC_TRANSFER:
      szParamLen = sizeof(mifare_param_value);
    break;

    // Please fix your code, you never should reach this statement
    default:
      return false;
    break;
  }

  // When available, copy the parameter bytes
  if (szParamLen) memcpy(abtCmd+5,(byte_t*)pmp,szParamLen);

  // Fire the mifare command
  if (!pn53x_transceive(pdi,abtCmd,5+szParamLen,abtRx,&szRxLen)) return false;

  // When we have executed a read command, copy the received bytes into the param
  if (mc == MC_READ && szRxLen == 17) memcpy(pmp->mpd.abtData,abtRx+1,16);

  // Command succesfully executed
  return true;
}

bool nfc_target_init(const dev_info* pdi, byte_t* pbtRx, size_t* pszRxBits)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  uint8_t ui8Bits;
  // Save the current configuration settings
  bool bCrc = pdi->bCrc;
  bool bPar = pdi->bPar;
  byte_t abtCmd[sizeof(pncmd_target_init)];
  memcpy(abtCmd,pncmd_target_init,sizeof(pncmd_target_init));

  // Clear the target init struct, reset to all zeros
  memset(abtCmd+2,0x00,37);

  // Set ATQA (SENS_RES)
  abtCmd[3] = 0x04;
  abtCmd[4] = 0x00;

  // Set SAK (SEL_RES)
  abtCmd[8] = 0x20;

  // Set UID
  abtCmd[5] = 0x00;
  abtCmd[6] = 0xb0;
  abtCmd[7] = 0x0b;

  // Make sure the CRC & parity are handled by the device, this is needed for target_init to work properly
  if (!bCrc) nfc_configure((dev_info*)pdi,DCO_HANDLE_CRC,true);
  if (!bPar) nfc_configure((dev_info*)pdi,DCO_HANDLE_PARITY,true);

  // Let the PN53X be activated by the RF level detector from power down mode
  if (!pn53x_set_reg(pdi,REG_CIU_TX_AUTO, SYMBOL_INITIAL_RF_ON,0x04)) return false;

  // Request the initialization as a target, we can not use pn53x_transceive() because
  // abtRx[0] contains the emulation mode (baudrate, 14443-4?, DEP and framing type)
  szRxLen = MAX_FRAME_LEN;
  if (!pdi->pdc->transceive(pdi->ds,abtCmd,39,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pdi,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // We are sure the parity is handled by the PN53X chip, so we handle it this way
  *pszRxBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;
  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,szRxLen-1);

  // Restore the CRC & parity setting to the original value (if needed)
  if (!bCrc) nfc_configure((dev_info*)pdi,DCO_HANDLE_CRC,false);
  if (!bPar) nfc_configure((dev_info*)pdi,DCO_HANDLE_PARITY,false);

  return true;
}

bool nfc_target_receive_bits(const dev_info* pdi, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szFrameBits;
  uint8_t ui8Bits;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pdi,pncmd_target_receive,2,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pdi,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  szFrameBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;

  // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
  // Check if we should recover the parity bits ourself
  if (!pdi->bPar)
  {
    // Unwrap the response frame
    pn53x_unwrap_frame(abtRx+1,szFrameBits,pbtRx,pszRxBits,pbtRxPar);
  } else {
    // Save the received bits
    *pszRxBits = szFrameBits;
    // Copy the received bytes
    memcpy(pbtRx,abtRx+1,szRxLen-1);
  }
  // Everyting seems ok, return true
  return true;
}

bool nfc_target_receive_dep_bytes(const dev_info* pdi, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pdi,pncmd_target_get_data,2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_receive_bytes(const dev_info* pdi, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pdi,pncmd_target_receive,2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_send_bits(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar)
{
  size_t szFrameBits = 0;
  size_t szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t abtCmd[sizeof(pncmd_target_send)];
  memcpy(abtCmd,pncmd_target_send,sizeof(pncmd_target_send));

  // Check if we should prepare the parity bits ourself
  if (!pdi->bPar)
  {
    // Convert data with parity to a frame
    pn53x_wrap_frame(pbtTx,szTxBits,pbtTxPar,abtCmd+2,&szFrameBits);
  } else {
    szFrameBits = szTxBits;
  }

  // Retrieve the leading bits
  ui8Bits = szFrameBits%8;

  // Get the amount of frame bytes + optional (1 byte if there are leading bits) 
  szFrameBytes = (szFrameBits/8)+((ui8Bits==0)?0:1);

  // When the parity is handled before us, we just copy the data
  if (pdi->bPar) memcpy(abtCmd+2,pbtTx,szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits(pdi,ui8Bits)) return false;

  // Try to send the bits to the reader
  if (!pn53x_transceive(pdi,abtCmd,szFrameBytes+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}


bool nfc_target_send_bytes(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxLen)
{
  byte_t abtCmd[sizeof(pncmd_target_send)];
  memcpy(abtCmd,pncmd_target_send,sizeof(pncmd_target_send));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pdi->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // Try to send the bits to the reader
  if (!pn53x_transceive(pdi,abtCmd,szTxLen+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_send_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const size_t szTxLen)
{
  byte_t abtCmd[sizeof(pncmd_target_set_data)];
  memcpy(abtCmd,pncmd_target_set_data,sizeof(pncmd_target_set_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pdi->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // Try to send the bits to the reader
  if (!pn53x_transceive(pdi,abtCmd,szTxLen+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}

const char* nfc_version(void)
{
  return PACKAGE_VERSION;
}


