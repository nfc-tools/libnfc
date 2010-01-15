/*-
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
 */
 
/** 
 * @file nfc.c
 * @brief NFC library implementation
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#include "chips.h"
#include "drivers.h"

#include <nfc/nfc-messages.h>

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

/**
 * @brief Probe for discoverable supported devices (ie. only available for some drivers)
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 */
void
nfc_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
  uint32_t uiDriver;
  size_t szN;

  *pszDeviceFound = 0;

  for (uiDriver=0; uiDriver<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDriver++)
  {
    if (drivers_callbacks_list[uiDriver].list_devices != NULL)
    {
      DBG("List avaible device using %s driver",drivers_callbacks_list[uiDriver].acDriver);
      szN = 0;
      if (drivers_callbacks_list[uiDriver].list_devices (pnddDevices + (*pszDeviceFound), szDevices - (*pszDeviceFound), &szN))
      {
        *pszDeviceFound += szN;
      }
    }
    else
    {
      DBG("No listing function avaible for %s driver",drivers_callbacks_list[uiDriver].acDriver);
    }
  }
}

/**
 * @brief Connect to a NFC device
 * @param pndd Device description if specific device is wanted, NULL otherwise
 * @return Returns pointer to a nfc_device_t struct if successfull; otherwise returns NULL value.
 *
 * If \a pndd is NULL, the first available NFC device is claimed by libnfc.
 * It will automatically search the system using all available drivers to determine a device is free.
 *
 * If \a pndd is passed then libnfc will try to claim the right device using information provided by this struct.
 *
 * When it has successfully claimed a NFC device, memory is allocated to save the device information. It will return a pointer to a nfc_device_t struct.
 * This pointer should be supplied by every next function of libnfc that should perform an action with this device.
 */
nfc_device_t* nfc_connect(nfc_device_desc_t* pndd)
{
  nfc_device_t* pnd = NULL;
  uint32_t uiDriver;
  byte_t abtFw[4];
  size_t szFwLen = sizeof(abtFw);

  // Search through the device list for an available device
  for (uiDriver=0; uiDriver<sizeof(drivers_callbacks_list)/sizeof(drivers_callbacks_list[0]); uiDriver++)
  {
    if(pndd == NULL) {
      // No device description specified: try to automatically claim a device
      if(drivers_callbacks_list[uiDriver].pick_device != NULL) {
        DBG("Autodetecting available devices using %s driver.", drivers_callbacks_list[uiDriver].acDriver);
        pndd = drivers_callbacks_list[uiDriver].pick_device ();

        if(pndd != NULL) {
          DBG("Auto-connecting to %s using %s driver", pndd->acDevice, drivers_callbacks_list[uiDriver].acDriver); 
          pnd = drivers_callbacks_list[uiDriver].connect(pndd);
          if(pnd == NULL) {
            DBG("No device available using %s driver",drivers_callbacks_list[uiDriver].acDriver);
            pndd = NULL;
          }

          free(pndd);
        }
      }
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
        DBG("Failed to get firmware revision for: %s", pnd->acName);
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

/**
 * @brief Disconnect from a NFC device
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * Initiator is disconnected and the device, including allocated nfc_device_t struct, is released.
 */
void nfc_disconnect(nfc_device_t* pnd)
{
  // Release and deselect all active communications
  nfc_initiator_deselect_tag(pnd);
  // Disable RF field to avoid heating
  nfc_configure(pnd,NDO_ACTIVATE_FIELD,false);
  // Disconnect, clean up and release the device 
  pnd->pdc->disconnect(pnd);
}

/**
 * @brief Configure advanced NFC device settings
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param ndo nfc_device_option_t struct that contains options to set to device
 * @param bEnable boolean
 *
 * Configures parameters and registers that control for example timing, modulation, frame and error handling.
 * There are different categories for configuring the PN53X chip features (handle, activate, infinite and accept).
 * These are defined to organize future settings that will become available when they are needed.
 */
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


/**
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * The NFC device is configured to function as RFID reader. After initialization it can be used to communicate to passive RFID tags and active NFC devices. The reader will act as initiator to communicate peer 2 peer (NFCIP) to other active NFC devices.
 */
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

/**
 * @brief Select a target and request active or passive mode for DEP (Data Exchange Protocol)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param im Desired modulation (NM_ACTIVE_DEP or NM_PASSIVE_DEP for active, respectively passive mode)
 * @param pbtPidData passive initiator data, 4 or 5 bytes long, (optional, only for NM_PASSIVE_DEP, can be NULL)
 * @param pbtNFCID3i the NFCID3, 10 bytes long, of the initiator (optional, can be NULL)
 * @param pbtGbData generic data of the initiator, max 48 bytes long, (optional, can be NULL)
 *
 * The NFC device will try to find the available target. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications.
 * @note nfc_target_info_t_dep will be returned when the target was acquired successfully.
 */
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

/**
 * @fn nfc_initiator_select_tag(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pti)
 * @brief Select a passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param im Desired modulation
 * @param pbtInitData Optional initiator data used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID.
 * @param szInitDataLen Length of initiator data \a pbtInitData.
 *
 * The NFC device will try to find the available passive tags. Some NFC devices are capable to emulate passive tags. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications. The chip needs to know with what kind of tag it is dealing with, therefore the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 * @note For every initial modulation type there is a different collection of information returned (in nfc_target_info_t pointer pti) They all fit in the data-type which is called nfc_target_info_t. This is a union which contains the tag information that belongs to the according initial modulation type.
 */
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

/**
 * @fn nfc_initiator_deselect_tag(const nfc_device_t* pnd);
 * @brief Deselect a selected passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * After selecting and communicating with a passive tag, this function could be used to deactivate and release the tag. This is very useful when there are multiple tags available in the field. It is possible to use the nfc_initiator_select_tag() function to select the first available tag, test it for the available features and support, deselect it and skip to the next tag until the correct tag is found.
 */
bool nfc_initiator_deselect_tag(const nfc_device_t* pnd)
{
  return (pn53x_transceive(pnd,pncmd_initiator_deselect,3,NULL,NULL));
}

/**
 * @fn nfc_initiator_transceive_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
 * @brief Transceive raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTxBits contains the length in bits.
 * @note For example the REQA (0x26) command (first anti-collision command of ISO14443-A) must be precise 7 bits long. This is not possible by using nfc_initiator_transceive_bytes(). With that function you can only communicate frames that consist of full bytes. When you send a full byte (8 bits + 1 parity) with the value of REQA (0x26), a tag will simply not respond. More information about this can be found in the anti-colision example.
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 * @note For example if you send the SELECT_ALL (0x93, 0x20) = [ 10010011, 00100000 ] command, you have to supply the following parity bytes (0x01, 0x00) to define the correct odd parity bits. This is only an example to explain how it works, if you just are sending two bytes with ISO14443-A compliant parity bits you better can use the nfc_initiator_transceive_bytes() function.
 * @returns The received response from the tag will be stored in the parameters (pbtRx, pszRxBits and pbtRxPar). They work the same way as the corresponding parameters for transmission.
 *
 * The NFC reader will transmit low-level messages where only the modulation is handled by the PN53X chip. Construction of the frame (data, CRC and parity) is completely done by libnfc. This can be very useful for testing purposes. Some protocols (e.g. MIFARE Classic) require to violate the ISO14443-A standard by sending incorrect parity and CRC bytes. Using this feature you are able to simulate these frames.
 */
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

/**
 * @brief Transceive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied (data) bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The difference between this function and nfc_initiator_transceive_bytes is that here pbtTx and pbtRx contain *only* the data sent and received and not any additional commands, that is all handled internally by the PN53X.
 */
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

/**
 * @brief Transceive byte and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The parity bits are handled by the PN53X chip. The CRC can be generated automatically or handled manually. Using this function, frames can be communicated very fast via the NFC reader to the tag. Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 * @warning The configuration option NDO_HANDLE_PARITY must be set to true (the default value).
 */
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

/**
 * @brief Execute a MIFARE Classic Command
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pmp Some commands need additional information. This information should be supplied in the mifare_param union.
 *
 * The specified MIFARE command will be executed on the tag. There are different commands possible, they all require the destination block number.
 * @note There are three different types of information (Authenticate, Data and Value).
 *
 * First an authentication must take place using Key A or B. It requires a 48 bit Key (6 bytes) and the UID. They are both used to initialize the internal cipher-state of the PN53X chip (http://libnfc.org/hardware/pn53x-chip). After a successful authentication it will be possible to execute other commands (e.g. Read/Write). The MIFARE Classic Specification (http://www.nxp.com/acrobat/other/identification/M001053_MF1ICS50_rev5_3.pdf) explains more about this process.
 */
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

/**
 * @brief Initialize NFC device as an emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This functionality allows the NFC device to act as an emulated tag. There seems to be quite some options available for this feature. Not all of the PN53X modulations are tested and documented at the moment. At the moment it could best be seen as a preliminary functionality.
 *
 * @warning Be aware that this function will wait (hang) until a command is received that is not part of the anti-collision. The RATS command for example would wake up the emulator. After this is received, the send and receive functions can be used.
 */
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

/**
 * @brief Receive bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function makes it possible to receive (raw) bit-frames. It returns all the messages that are stored in the FIFO buffer of the PN53X chip. It does not require to send any frame and thereby could be used to snoop frames that are transmitted by a nearby reader. Check out the NDO_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted frames.
 */
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

/**
 * @brief Receive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received data from a nearby reader. The difference between this function and nfc_target_receive_bytes is that here pbtRx contains *only* the data received and not any additional commands, that is all handled internally by the PN53X.
 */
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

/**
 * @brief Receive bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received frames from a nearby reader.
 */
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

/**
 * @brief Send raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function can be used to transmit (raw) bit-frames to the reader.
 */
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


/**
 * @brief Send bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate byte frames and APDU responses to the reader, this function could be used.
 */
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

/**
 * @brief Send data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate data to the reader, this function could be used. The difference between this function and nfc_target_send_bytes is that here pbtTx contains *only* the data sent and not any additional commands, that is all handled internally by the PN53X.
 */
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

/* Special data accessors */

/**
 * @brief Returns the device name
 * @return Returns a string with the device name ( MUST be freed with free() )
 */
const char* nfc_device_name(nfc_device_t* pnd)
{
  return pnd->acName;
}

/* Misc. functions */

/**
 * @brief Returns the library version
 * @return Returns a string with the library version
 */
const char* nfc_version(void)
{
	#ifdef SVN_REVISION
	return PACKAGE_VERSION" (r"SVN_REVISION")";
	#else
	return PACKAGE_VERSION;
	#endif // SVN_REVISION
}

