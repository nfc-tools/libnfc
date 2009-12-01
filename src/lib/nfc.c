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
 * @brief NFC library implementation
 */


#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#include "chips.h"
#include "drivers.h"

#include <nfc/nfc-messages.h>

#include "../../config.h"

nfc_device_desc_t * nfc_pick_device (void);

// PN53X configuration
extern const byte_t pncmd_get_firmware_version       [  2];
extern const byte_t pncmd_get_general_status         [  2];
extern const byte_t pncmd_get_register               [  4];
extern const byte_t pncmd_set_register               [  5];
extern const byte_t pncmd_set_parameters             [  3];
extern const byte_t pncmd_rf_configure               [ 14];

// Reader
extern const byte_t pncmd_initiator_list_passive        [264];
extern const byte_t pncmd_initiator_jump_for_dep        [ 68];
extern const byte_t pncmd_initiator_select              [  3];
extern const byte_t pncmd_initiator_deselect            [  3];
extern const byte_t pncmd_initiator_release             [  3];
extern const byte_t pncmd_initiator_set_baud_rate       [  5];
extern const byte_t pncmd_initiator_exchange_data       [265];
extern const byte_t pncmd_initiator_exchange_raw_data   [266];
extern const byte_t pncmd_initiator_auto_poll           [  5];

// Target
extern const byte_t pncmd_target_get_data            [  2];
extern const byte_t pncmd_target_set_data            [264];
extern const byte_t pncmd_target_init                [ 39];
extern const byte_t pncmd_target_virtual_card        [  4];
extern const byte_t pncmd_target_receive             [  2];
extern const byte_t pncmd_target_send                [264];
extern const byte_t pncmd_target_get_status          [  2];

nfc_device_desc_t *
nfc_pick_device (void)
{
  uint32_t uiDriver;
  nfc_device_desc_t *nddRes;

  for (uiDriver=0; uiDriver<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDriver++)
  {
    if (drivers_callbacks_list[uiDriver].pick_device != NULL)
    {
      nddRes = drivers_callbacks_list[uiDriver].pick_device ();
      if (nddRes != NULL) return nddRes;
    }
  }

  return NULL;
}

void
nfc_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
  uint32_t uiDriver;

  *pszDeviceFound = 0;

  for (uiDriver=0; uiDriver<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDriver++)
  {
    if (drivers_callbacks_list[uiDriver].list_devices != NULL)
    {
      DBG("Checking driver: %s",drivers_callbacks_list[uiDriver]);
      size_t szN = 0;
      if (drivers_callbacks_list[uiDriver].list_devices (pnddDevices + (*pszDeviceFound), szDevices - (*pszDeviceFound), &szN))
      {
        *pszDeviceFound += szN;
      }
    }
    #ifdef DEBUG
    else
      DBG("Not checking driver: %s",drivers_callbacks_list[uiDriver]);
    #endif
  }
}

nfc_device_t* nfc_connect(nfc_device_desc_t* pndd)
{
  nfc_device_t* pnd;
  uint32_t uiDriver;
  byte_t abtFw[4];
  size_t szFwLen = sizeof(abtFw);

  // Search through the device list for an available device
  for (uiDriver=0; uiDriver<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDriver++)
  {
    if (pndd == NULL) {
      // No device description specified: try to automatically claim a device
      DBG("%s","Autodetecting available devices...");
      pndd = drivers_callbacks_list[uiDriver].pick_device ();
      pnd = drivers_callbacks_list[uiDriver].connect(pndd);
    } else {
      // Specific device is requested: using device description pndd
      if( 0 != strcmp(drivers_callbacks_list[uiDriver].acDriver, pndd->pcDriver ) )
      {
        DBG("Looking for %s, found %s... Skip it.", pndd->pcDriver, drivers_callbacks_list[uiDriver].acDriver);
        continue;
      } else {
        DBG("Looking for %s, found %s... Use it.", pndd->pcDriver, drivers_callbacks_list[uiDriver].acDriver);
        pnd = drivers_callbacks_list[uiDriver].connect(pndd);
      }
    }

    // Test if the connection was successful
    if (pnd != NULL)
    {
      DBG("[%s] has been claimed.", pnd->acName);
      // Great we have claimed a device
      pnd->pdc = &(drivers_callbacks_list[uiDriver]);

      // Try to retrieve PN53x chip revision
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pnd->pdc->transceive(pnd->nds,pncmd_get_firmware_version,2,abtFw,&szFwLen))
      {
        // Failed to get firmware revision??, whatever...let's disconnect and clean up and return err
        ERR("Failed to get firmware revision for: %s", pnd->acName);
        pnd->pdc->disconnect(pnd);
        return NULL;
      }

      // Add the firmware revision to the device name, PN531 gives 2 bytes info, but PN532 gives 4
      switch(pnd->nc)
      {
        case NC_PN531: sprintf(pnd->acName,"%s - PN531 v%d.%d",pnd->acName,abtFw[0],abtFw[1]); break;
        case NC_PN532: sprintf(pnd->acName,"%s - PN532 v%d.%d (0x%02x)",pnd->acName,abtFw[1],abtFw[2],abtFw[3]); break;
        case NC_PN533: sprintf(pnd->acName,"%s - PN533 v%d.%d (0x%02x)",pnd->acName,abtFw[1],abtFw[2],abtFw[3]); break;
      }

      // Reset the ending transmission bits register, it is unknown what the last tranmission used there
      if (!pn53x_set_reg(pnd,REG_CIU_BIT_FRAMING,SYMBOL_TX_LAST_BITS,0x00)) return NULL;

      // Make sure we reset the CRC and parity to chip handling.
      if (!nfc_configure(pnd,NDO_HANDLE_CRC,true)) return NULL;
      if (!nfc_configure(pnd,NDO_HANDLE_PARITY,true)) return NULL;

      // Deactivate the CRYPTO1 chiper, it may could cause problems when still active
      if (!nfc_configure(pnd,NDO_ACTIVATE_CRYPTO1,false)) return NULL;

      return pnd;
    } else {
      DBG("No device found using driver: %s", drivers_callbacks_list[uiDriver].acDriver);
    }
  }
  // To bad, no reader is ready to be claimed
  return NULL;
}

void nfc_disconnect(nfc_device_t* pnd)
{
  // Release and deselect all active communications
  nfc_initiator_deselect_tag(pnd);
  // Disable RF field to avoid heating
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);
  // Disconnect, clean up and release the device 
  pnd->pdc->disconnect(pnd);
}

bool nfc_configure(nfc_device_t* pnd, const nfc_device_option_t dco, const bool bEnable)
{
  byte_t btValue;
  byte_t abtCmd[sizeof(pncmd_rf_configure)];
  memcpy(abtCmd,pncmd_rf_configure,sizeof(pncmd_rf_configure));

  // Make sure we are dealing with a active device
  if (!pnd->bActive) return false;

  switch(dco)
  {
    case NDO_HANDLE_CRC:
      // Enable or disable automatic receiving/sending of CRC bytes
      // TX and RX are both represented by the symbol 0x80
      btValue = (bEnable) ? 0x80 : 0x00;
      if (!pn53x_set_reg(pnd,REG_CIU_TX_MODE,SYMBOL_TX_CRC_ENABLE,btValue)) return false;
      if (!pn53x_set_reg(pnd,REG_CIU_RX_MODE,SYMBOL_RX_CRC_ENABLE,btValue)) return false;
      pnd->bCrc = bEnable;
    break;

    case NDO_HANDLE_PARITY:
      // Handle parity bit by PN53X chip or parse it as data bit
      btValue = (bEnable) ? 0x00 : SYMBOL_PARITY_DISABLE;
      if (!pn53x_set_reg(pnd,REG_CIU_MANUAL_RCV,SYMBOL_PARITY_DISABLE,btValue)) return false;
      pnd->bPar = bEnable;
    break;

    case NDO_ACTIVATE_FIELD:
      abtCmd[2] = RFCI_FIELD;
      abtCmd[3] = (bEnable) ? 1 : 0;
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pnd->pdc->transceive(pnd->nds,abtCmd,4,NULL,NULL)) return false;
    break;

    case NDO_ACTIVATE_CRYPTO1:
      btValue = (bEnable) ? SYMBOL_MF_CRYPTO1_ON : 0x00;
      if (!pn53x_set_reg(pnd,REG_CIU_STATUS2,SYMBOL_MF_CRYPTO1_ON,btValue)) return false;
    break;

    case NDO_INFINITE_SELECT:
      // Retry format: 0x00 means only 1 try, 0xff means infinite
      abtCmd[2] = RFCI_RETRY_SELECT;
      abtCmd[3] = (bEnable) ? 0xff : 0x00; // MxRtyATR, default: active = 0xff, passive = 0x02
      abtCmd[4] = (bEnable) ? 0xff : 0x00; // MxRtyPSL, default: 0x01
      abtCmd[5] = (bEnable) ? 0xff : 0x00; // MxRtyPassiveActivation, default: 0xff
      // We can not use pn53x_transceive() because abtRx[0] gives no status info
      if (!pnd->pdc->transceive(pnd->nds,abtCmd,6,NULL,NULL)) return false;
    break;

    case NDO_ACCEPT_INVALID_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_NO_ERROR : 0x00;
      if (!pn53x_set_reg(pnd,REG_CIU_RX_MODE,SYMBOL_RX_NO_ERROR,btValue)) return false;
    break;

    case NDO_ACCEPT_MULTIPLE_FRAMES:
      btValue = (bEnable) ? SYMBOL_RX_MULTIPLE : 0x00;
      if (!pn53x_set_reg(pnd,REG_CIU_RX_MODE,SYMBOL_RX_MULTIPLE,btValue)) return false;
    return true;

    break;
  }

  // When we reach this, the configuration is completed and succesful
  return true;
}

bool nfc_initiator_init(const nfc_device_t* pnd)
{
  // Make sure we are dealing with a active device
  if (!pnd->bActive) return false;

  // Set the PN53X to force 100% ASK Modified miller decoding (default for 14443A cards)
  if (!pn53x_set_reg(pnd,REG_CIU_TX_AUTO,SYMBOL_FORCE_100_ASK,0x40)) return false;

  // Configure the PN53X to be an Initiator or Reader/Writer
  if (!pn53x_set_reg(pnd,REG_CIU_CONTROL,SYMBOL_INITIATOR,0x10)) return false;

  return true;
}

bool nfc_initiator_select_dep_target(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t* pnti)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t offset;
  byte_t abtCmd[sizeof(pncmd_initiator_jump_for_dep)];
  memcpy(abtCmd,pncmd_initiator_jump_for_dep,sizeof(pncmd_initiator_jump_for_dep));

  if(nmInitModulation == NM_ACTIVE_DEP) {
    abtCmd[2] = 0x01; /* active DEP */
  }
  abtCmd[3] = 0x00; /* baud rate = 106kbps */

  offset = 5;
  if(pbtPidData && nmInitModulation != NM_ACTIVE_DEP) { /* can't have passive initiator data when using active mode */
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
  if (!pn53x_transceive(pnd,abtCmd,5+szPidDataLen+szNFCID3iDataLen+szGbDataLen,abtRx,&szRxLen)) return false;

  // Make sure one target has been found, the PN53X returns 0x00 if none was available
  if (abtRx[1] != 1) return false;

  // Is a target info struct available
  if (pnti)
  {
    memcpy(pnti->ndi.NFCID3i,abtRx+2,10);
    pnti->ndi.btDID = abtRx[12];
    pnti->ndi.btBSt = abtRx[13];
    pnti->ndi.btBRt = abtRx[14];
  }
  return true;
}

bool nfc_initiator_select_tag(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pnti)
{
  byte_t abtInit[MAX_FRAME_LEN];
  size_t szInitLen;
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_list_passive)];
  memcpy(abtCmd,pncmd_initiator_list_passive,sizeof(pncmd_initiator_list_passive));

  // Make sure we are dealing with a active device
  if (!pnd->bActive) return false;

  abtCmd[2] = 1;  // MaxTg, we only want to select 1 tag at the time
  abtCmd[3] = nmInitModulation; // BrTy, the type of init modulation used for polling a passive tag

  switch(nmInitModulation)
  {
    case NM_ISO14443A_106:
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
  if (!pnd->pdc->transceive(pnd->nds,abtCmd,4+szInitLen,abtRx,&szRxLen)) return false;

  // Make sure one tag has been found, the PN53X returns 0x00 if none was available
  if (abtRx[0] != 1) return false;

  // Is a tag info struct available
  if (pnti)
  {
    // Fill the tag info struct with the values corresponding to this init modulation
    switch(nmInitModulation)
    {
      case NM_ISO14443A_106:
        // Somehow they switched the lower and upper ATQA bytes around for the PN531 chipset
        if (pnd->nc == NC_PN531)
        {
          pnti->nai.abtAtqa[0] = abtRx[3];
          pnti->nai.abtAtqa[1] = abtRx[2];
        } else {
          memcpy(pnti->nai.abtAtqa,abtRx+2,2);
        }
        pnti->nai.btSak = abtRx[4];
        // Copy the NFCID1
        pnti->nai.szUidLen = abtRx[5];
        memcpy(pnti->nai.abtUid,abtRx+6,pnti->nai.szUidLen);
        // Did we received an optional ATS (Smardcard ATR)
        if (szRxLen > pnti->nai.szUidLen+6)
        {
          pnti->nai.szAtsLen = abtRx[pnti->nai.szUidLen+6];
          memcpy(pnti->nai.abtAts,abtRx+pnti->nai.szUidLen+6,pnti->nai.szAtsLen);
        } else {
          pnti->nai.szAtsLen = 0;
        }
      break;

      case NM_FELICA_212:
      case NM_FELICA_424:
        // Store the mandatory info
        pnti->nfi.szLen = abtRx[2];
        pnti->nfi.btResCode = abtRx[3];
        // Copy the NFCID2t
        memcpy(pnti->nfi.abtId,abtRx+4,8);
        // Copy the felica padding
        memcpy(pnti->nfi.abtPad,abtRx+12,8);
        // Test if the System code (SYST_CODE) is available
        if (szRxLen > 20)
        {
          memcpy(pnti->nfi.abtSysCode,abtRx+20,2);  
        }
      break;

      case NM_ISO14443B_106:
        // Store the mandatory info
        memcpy(pnti->nbi.abtAtqb,abtRx+2,12);
        // Ignore the 0x1D byte, and just store the 4 byte id
        memcpy(pnti->nbi.abtId,abtRx+15,4);
        pnti->nbi.btParam1 = abtRx[19];
        pnti->nbi.btParam2 = abtRx[20];
        pnti->nbi.btParam3 = abtRx[21];
        pnti->nbi.btParam4 = abtRx[22];
        // Test if the Higher layer (INF) is available
        if (szRxLen > 22)
        {
          pnti->nbi.szInfLen = abtRx[23];
          memcpy(pnti->nbi.abtInf,abtRx+24,pnti->nbi.szInfLen);
        } else {
          pnti->nbi.szInfLen = 0;
        }
      break;

      case NM_JEWEL_106:
        // Store the mandatory info
        memcpy(pnti->nji.btSensRes,abtRx+2,2);
        memcpy(pnti->nji.btId,abtRx+4,4);
      break;

      default:
        // Should not be possible, so whatever...
      break;
    }
  }
  return true;
}

bool nfc_initiator_deselect_tag(const nfc_device_t* pnd)
{
  return (pn53x_transceive(pnd,pncmd_initiator_deselect,3,NULL,NULL));
}

bool nfc_initiator_transceive_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szFrameBits = 0;
  size_t szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_raw_data,sizeof(pncmd_initiator_exchange_raw_data));

  // Check if we should prepare the parity bits ourself
  if (!pnd->bPar)
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
  if (pnd->bPar) memcpy(abtCmd+2,pbtTx,szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits(pnd,ui8Bits)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pnd,abtCmd,szFrameBytes+2,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pnd,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  szFrameBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;

  // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
  // Check if we should recover the parity bits ourself
  if (!pnd->bPar)
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

bool nfc_initiator_transceive_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_data,sizeof(pncmd_initiator_exchange_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) return false;

  // Copy the data into the command frame
  abtCmd[2] = 1; /* target number */
  memcpy(abtCmd+3,pbtTx,szTxLen);

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits(pnd,0)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pnd,abtCmd,szTxLen+3,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everything went successful
  return true;
}

bool nfc_initiator_transceive_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_raw_data,sizeof(pncmd_initiator_exchange_raw_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits(pnd,0)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
  if (!pn53x_transceive(pnd,abtCmd,szTxLen+2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everything went successful
  return true;
}

bool nfc_initiator_mifare_cmd(const nfc_device_t* pnd, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szParamLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_data)];
  memcpy(abtCmd,pncmd_initiator_exchange_data,sizeof(pncmd_initiator_exchange_data));

  // Make sure we are dealing with a active device
  if (!pnd->bActive) return false;

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
  if (!pn53x_transceive(pnd,abtCmd,5+szParamLen,abtRx,&szRxLen)) return false;

  // When we have executed a read command, copy the received bytes into the param
  if (mc == MC_READ && szRxLen == 17) memcpy(pmp->mpd.abtData,abtRx+1,16);

  // Command succesfully executed
  return true;
}

bool nfc_target_init(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  uint8_t ui8Bits;
  // Save the current configuration settings
  bool bCrc = pnd->bCrc;
  bool bPar = pnd->bPar;
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
  if (!bCrc) nfc_configure((nfc_device_t*)pnd,NDO_HANDLE_CRC,true);
  if (!bPar) nfc_configure((nfc_device_t*)pnd,NDO_HANDLE_PARITY,true);

  // Let the PN53X be activated by the RF level detector from power down mode
  if (!pn53x_set_reg(pnd,REG_CIU_TX_AUTO, SYMBOL_INITIAL_RF_ON,0x04)) return false;

  // Request the initialization as a target, we can not use pn53x_transceive() because
  // abtRx[0] contains the emulation mode (baudrate, 14443-4?, DEP and framing type)
  szRxLen = MAX_FRAME_LEN;
  if (!pnd->pdc->transceive(pnd->nds,abtCmd,39,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pnd,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // We are sure the parity is handled by the PN53X chip, so we handle it this way
  *pszRxBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;
  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,szRxLen-1);

  // Restore the CRC & parity setting to the original value (if needed)
  if (!bCrc) nfc_configure((nfc_device_t*)pnd,NDO_HANDLE_CRC,false);
  if (!bPar) nfc_configure((nfc_device_t*)pnd,NDO_HANDLE_PARITY,false);

  return true;
}

bool nfc_target_receive_bits(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szFrameBits;
  uint8_t ui8Bits;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pnd,pncmd_target_receive,2,abtRx,&szRxLen)) return false;

  // Get the last bit-count that is stored in the received byte 
  ui8Bits = pn53x_get_reg(pnd,REG_CIU_CONTROL) & SYMBOL_RX_LAST_BITS;

  // Recover the real frame length in bits
  szFrameBits = ((szRxLen-1-((ui8Bits==0)?0:1))*8)+ui8Bits;

  // Ignore the status byte from the PN53X here, it was checked earlier in pn53x_transceive()
  // Check if we should recover the parity bits ourself
  if (!pnd->bPar)
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

bool nfc_target_receive_dep_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pnd,pncmd_target_get_data,2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_receive_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pnd,pncmd_target_receive,2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_send_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar)
{
  size_t szFrameBits = 0;
  size_t szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t abtCmd[sizeof(pncmd_target_send)];
  memcpy(abtCmd,pncmd_target_send,sizeof(pncmd_target_send));

  // Check if we should prepare the parity bits ourself
  if (!pnd->bPar)
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
  if (pnd->bPar) memcpy(abtCmd+2,pbtTx,szFrameBytes);

  // Set the amount of transmission bits in the PN53X chip register
  if (!pn53x_set_tx_bits(pnd,ui8Bits)) return false;

  // Try to send the bits to the reader
  if (!pn53x_transceive(pnd,abtCmd,szFrameBytes+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}


bool nfc_target_send_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen)
{
  byte_t abtCmd[sizeof(pncmd_target_send)];
  memcpy(abtCmd,pncmd_target_send,sizeof(pncmd_target_send));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // Try to send the bits to the reader
  if (!pn53x_transceive(pnd,abtCmd,szTxLen+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}

bool nfc_target_send_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen)
{
  byte_t abtCmd[sizeof(pncmd_target_set_data)];
  memcpy(abtCmd,pncmd_target_set_data,sizeof(pncmd_target_set_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) return false;

  // Copy the data into the command frame
  memcpy(abtCmd+2,pbtTx,szTxLen);

  // Try to send the bits to the reader
  if (!pn53x_transceive(pnd,abtCmd,szTxLen+2,NULL,NULL)) return false;

  // Everyting seems ok, return true
  return true;
}

const char* nfc_version(void)
{
#ifdef SVN_REVISION
  return PACKAGE_VERSION" (r"SVN_REVISION")";
#else
  return PACKAGE_VERSION;
#endif // SVN_REVISION
}


