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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <nfc/nfc.h>
// FIXME: WTF are doing debug macros in this file?
#include <nfc/nfc-messages.h>

#include "pn53x.h"
#include "../mirror-subr.h"

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

static const byte_t pn53x_ack_frame[]  = { 0x00,0x00,0xff,0x00,0xff,0x00 };
static const byte_t pn53x_nack_frame[] = { 0x00,0x00,0xff,0xff,0x00,0x00 };
static const byte_t pn53x_error_frame[] = { 0x00,0x00,0xff,0x01,0xff,0x7f,0x81,0x00 };

// XXX: Is this function correctly named ?
bool pn53x_transceive_check_ack_frame_callback(nfc_device_t* pnd, const byte_t *pbtRxFrame, const size_t szRxFrameLen)
{
  if (szRxFrameLen >= sizeof (pn53x_ack_frame)) {
    if (0 == memcmp (pbtRxFrame, pn53x_ack_frame, sizeof (pn53x_ack_frame))) {
      DBG("%s", "PN53x ACKed");
      return true;
    } else if (0 == memcmp (pbtRxFrame, pn53x_nack_frame, sizeof (pn53x_nack_frame))) {
      DBG("%s", "PN53x NACKed");
      // TODO: Try to recover
      // A counter could allow the command to be sent again (e.g. max 3 times)
      pnd->iLastError = DENACK;
      return false;
    }
  }
  pnd->iLastError = DEACKMISMATCH;
  ERR("%s", "Unexpected PN53x reply!");
#if defined(DEBUG)
  // coredump so that we can have a backtrace about how this code was reached.
  abort();
#endif
  return false;
}

bool pn53x_transceive_check_error_frame_callback(nfc_device_t* pnd, const byte_t *pbtRxFrame, const size_t szRxFrameLen)
{
  if (szRxFrameLen >= sizeof (pn53x_error_frame)) {
    if (0 == memcmp (pbtRxFrame, pn53x_error_frame, sizeof (pn53x_error_frame))) {
      DBG("%s", "PN53x sent an error frame");
      pnd->iLastError = DEISERRFRAME;
      return false;
    }
  }

  return true;
}

bool pn53x_transceive(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
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
  if (!pnd->pdc->transceive(pnd,pbtTx,szTxLen,pbtRx,pszRxLen)) return false;

  switch (pbtTx[1]) {
      case 0x16:	// PowerDown
      case 0x40:	// InDataExchange
      case 0x42:	// InCommunicateThru
      case 0x44:	// InDeselect
      case 0x46:	// InJumpForPSL
      case 0x4e:	// InPSL
      case 0x50:	// InATR
      case 0x52:	// InRelease
      case 0x54:	// InSelect
      case 0x56:	// InJumpForDEP
      case 0x86:	// TgGetData
      case 0x88:	// TgGetInitiatorCommand
      case 0x8e:	// TgSetData
      case 0x90:	// TgResponseToInitiator
      case 0x92:	// TgSetGeneralBytes
      case 0x94:	// TgSetMetaData
	  pnd->iLastError = pbtRx[0] & 0x3f;
	  break;
      default:
	  pnd->iLastError = 0;
  }

  return (0 == pnd->iLastError);
}

byte_t pn53x_get_reg(nfc_device_t* pnd, uint16_t ui16Reg)
{
  uint8_t ui8Value;
  size_t szValueLen = 1;
  byte_t abtCmd[sizeof(pncmd_get_register)];
  memcpy(abtCmd,pncmd_get_register,sizeof(pncmd_get_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  pn53x_transceive(pnd,abtCmd,4,&ui8Value,&szValueLen);
  return ui8Value;
}

bool pn53x_set_reg(nfc_device_t* pnd, uint16_t ui16Reg, uint8_t ui8SybmolMask, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_register)];
  memcpy(abtCmd,pncmd_set_register,sizeof(pncmd_set_register));

  abtCmd[2] = ui16Reg >> 8;
  abtCmd[3] = ui16Reg & 0xff;
  abtCmd[4] = ui8Value | (pn53x_get_reg(pnd,ui16Reg) & (~ui8SybmolMask));
  return pn53x_transceive(pnd,abtCmd,5,NULL,NULL);
}

bool pn53x_set_parameters(nfc_device_t* pnd, uint8_t ui8Value)
{
  byte_t abtCmd[sizeof(pncmd_set_parameters)];
  memcpy(abtCmd,pncmd_set_parameters,sizeof(pncmd_set_parameters));

  abtCmd[2] = ui8Value;
  return pn53x_transceive(pnd,abtCmd,3,NULL,NULL);
}

bool pn53x_set_tx_bits(nfc_device_t* pnd, uint8_t ui8Bits)
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

bool 
pn53x_decode_target_data(const byte_t* pbtRawData, size_t szDataLen, nfc_chip_t nc, nfc_target_type_t ntt, nfc_target_info_t* pnti)
{
  uint8_t ui8AttribResLen;
  switch(ntt) {
    case NTT_MIFARE:
    case NTT_GENERIC_PASSIVE_106:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;
      
      // Somehow they switched the lower and upper ATQA bytes around for the PN531 chipset
      if (nc == NC_PN531) {
        pnti->nai.abtAtqa[1] = *(pbtRawData++);
        pnti->nai.abtAtqa[0] = *(pbtRawData++);
      } else {
        pnti->nai.abtAtqa[0] = *(pbtRawData++);
        pnti->nai.abtAtqa[1] = *(pbtRawData++);
      }
      pnti->nai.btSak = *(pbtRawData++);
      // Copy the NFCID1
      pnti->nai.szUidLen = *(pbtRawData++);
      memcpy(pnti->nai.abtUid, pbtRawData, pnti->nai.szUidLen);
      pbtRawData += pnti->nai.szUidLen;
      
      // Did we received an optional ATS (Smardcard ATR)
      if (szDataLen > (pnti->nai.szUidLen + 5)) {
        pnti->nai.szAtsLen = ((*(pbtRawData++)) - 1); // In pbtRawData, ATS Length byte is counted in ATS Frame.
        memcpy(pnti->nai.abtAts, pbtRawData, pnti->nai.szAtsLen);
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
      
    case NTT_ISO14443B_106:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;
      
      // Store the mandatory info
      memcpy(pnti->nbi.abtAtqb, pbtRawData, 12);
      pbtRawData += 12;
      
      // Store temporarily the ATTRIB_RES length
      ui8AttribResLen = *(pbtRawData++);

      // Store the 4 bytes ID
      memcpy(pnti->nbi.abtId, pbtRawData,4);
      pbtRawData += 4;

      pnti->nbi.btParam1 = *(pbtRawData++);
      pnti->nbi.btParam2 = *(pbtRawData++);
      pnti->nbi.btParam3 = *(pbtRawData++);
      pnti->nbi.btParam4 = *(pbtRawData++);
      
      // Test if the Higher layer (INF) is available
      if (ui8AttribResLen > 8) {
        pnti->nbi.szInfLen = *(pbtRawData++);
        memcpy(pnti->nbi.abtInf, pbtRawData, pnti->nbi.szInfLen);
      } else {
        pnti->nbi.szInfLen = 0;
      }
      break;
      
    case NTT_FELICA_212:
    case NTT_FELICA_424:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;
      
      // Store the mandatory info
      pnti->nfi.szLen = *(pbtRawData++);
      pnti->nfi.btResCode = *(pbtRawData++);
      // Copy the NFCID2t
      memcpy(pnti->nfi.abtId, pbtRawData, 8);
      pbtRawData += 8;
      // Copy the felica padding
      memcpy(pnti->nfi.abtPad, pbtRawData, 8);
      pbtRawData += 8;
      // Test if the System code (SYST_CODE) is available
      if (pnti->nfi.szLen > 18)
      {
        memcpy(pnti->nfi.abtSysCode, pbtRawData, 2);
      }
      break;
    case NTT_JEWEL_106:
      // We skip the first byte: its the target number (Tg)
      pbtRawData++;
      
      // Store the mandatory info
      memcpy(pnti->nji.btSensRes, pbtRawData, 2);
      pbtRawData += 2;
      memcpy(pnti->nji.btId, pbtRawData, 4);
      break;
    default:
      return false;
      break;
  }
  return true;
}

/**
 * @brief C wrapper to InListPassiveTarget command
 * @return true if command is successfully sent
 *
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param nmInitModulation Desired modulation
 * @param pbtInitiatorData Optional initiator data used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID
 * @param szInitiatorDataLen Length of initiator data \a pbtInitiatorData
 * @param pbtTargetsData pointer on a pre-allocated byte array to receive TargetData[n] as described in pn53x user manual
 * @param pszTargetsData size_t pointer where size of \a pbtTargetsData will be written
 *
 * @note Selected targets count can be found in \a pbtTargetsData[0] if available (i.e. \a pszTargetsData content is more than 0)
 * @note To decode theses TargetData[n], there is @fn pn53x_decode_target_data
 */
bool
pn53x_InListPassiveTarget(nfc_device_t* pnd,
                          const nfc_modulation_t nmInitModulation, const byte_t szMaxTargets,
                          const byte_t* pbtInitiatorData, const size_t szInitiatorDataLen,
                          byte_t* pbtTargetsData, size_t* pszTargetsData)
{
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_list_passive)];
  memcpy(abtCmd,pncmd_initiator_list_passive,sizeof(pncmd_initiator_list_passive));

  // FIXME PN531 doesn't support all available modulations
  abtCmd[2] = szMaxTargets;  // MaxTg
  abtCmd[3] = nmInitModulation; // BrTy, the type of init modulation used for polling a passive tag

  // Set the optional initiator data (used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID).
  if (pbtInitiatorData) memcpy(abtCmd+4,pbtInitiatorData,szInitiatorDataLen);

  // Try to find a tag, call the tranceive callback function of the current device
  szRxLen = MAX_FRAME_LEN;
  if(pn53x_transceive(pnd,abtCmd,4+szInitiatorDataLen,pbtTargetsData,&szRxLen)) {
    *pszTargetsData = szRxLen;
    return true;
  } else {
    return false;
  }
}

bool
pn53x_InDeselect(nfc_device_t* pnd, const uint8_t ui8Target)
{
  byte_t abtCmd[sizeof(pncmd_initiator_deselect)];
  memcpy(abtCmd,pncmd_initiator_deselect,sizeof(pncmd_initiator_deselect));
  abtCmd[2] = ui8Target;
  
  return(pn53x_transceive(pnd,abtCmd,sizeof(abtCmd),NULL,NULL));
}

bool
pn53x_InRelease(nfc_device_t* pnd, const uint8_t ui8Target)
{
  byte_t abtCmd[sizeof(pncmd_initiator_release)];
  memcpy(abtCmd,pncmd_initiator_release,sizeof(pncmd_initiator_release));
  abtCmd[2] = ui8Target;
  
  return(pn53x_transceive(pnd,abtCmd,sizeof(abtCmd),NULL,NULL));
}

bool
pn53x_InAutoPoll(nfc_device_t* pnd,
                 const nfc_target_type_t* pnttTargetTypes, const size_t szTargetTypes,
                 const byte_t btPollNr, const byte_t btPeriod,
                 nfc_target_t* pntTargets, size_t* pszTargetFound)
{
  size_t szTxInAutoPoll, n, szRxLen;
  byte_t abtRx[MAX_FRAME_LEN];
  bool res;
  byte_t *pbtTxInAutoPoll;

  if(pnd->nc != NC_PN532) {
    // This function is not supported by pn531 neither pn533
    pnd->iLastError = DENOTSUP;
    return false;
  }

  // InAutoPoll frame looks like this { 0xd4, 0x60, 0x0f, 0x01, 0x00 } => { direction, command, pollnr, period, types... }
  szTxInAutoPoll = 4 + szTargetTypes;
  pbtTxInAutoPoll = malloc( szTxInAutoPoll );
  pbtTxInAutoPoll[0] = 0xd4;
  pbtTxInAutoPoll[1] = 0x60;
  pbtTxInAutoPoll[2] = btPollNr;
  pbtTxInAutoPoll[3] = btPeriod;
  for(n=0; n<szTargetTypes; n++) {
    pbtTxInAutoPoll[4+n] = pnttTargetTypes[n];
  }

  szRxLen = MAX_FRAME_LEN;
  res = pnd->pdc->transceive(pnd, pbtTxInAutoPoll, szTxInAutoPoll, abtRx, &szRxLen);

  if((szRxLen == 0)||(res == false)) {
    return false;
  } else {
    *pszTargetFound = abtRx[0];
    if( *pszTargetFound ) {
      uint8_t ln;
      byte_t* pbt = abtRx + 1;
      /* 1st target */
      // Target type
      pntTargets[0].ntt = *(pbt++);
      // AutoPollTargetData length
      ln = *(pbt++);
      pn53x_decode_target_data(pbt, ln, pnd->nc, pntTargets[0].ntt, &(pntTargets[0].nti));
      pbt += ln;

      if(abtRx[0] > 1) {
        /* 2nd target */
        // Target type
        pntTargets[1].ntt = *(pbt++);
        // AutoPollTargetData length
        ln = *(pbt++);
        pn53x_decode_target_data(pbt, ln, pnd->nc, pntTargets[1].ntt, &(pntTargets[1].nti));
      }
    }
  }
  return true;
}

static struct sErrorMessage {
  int iErrorCode;
  const char *pcErrorMsg;
} sErrorMessages[] = {
  /* Chip-level errors */
  { 0x00, "Success" },
  { 0x01, "Timeout" },
  { 0x02, "CRC Error" },
  { 0x03, "Parity Error" },
  { 0x04, "Erroneous Bit Count" },
  { 0x05, "Framing Error" },
  { 0x06, "Bit-collision" },
  { 0x07, "Buffer Too Small" },
  { 0x09, "Buffer Overflow" },
  { 0x0a, "Timeout" },
  { 0x0b, "Protocol Error" },
  { 0x0d, "Overheating" },
  { 0x0e, "Internal Buffer overflow." },
  { 0x10, "Invalid Parameter" },
  /* DEP Errors */
  { 0x12, "Unknown DEP Command" },
  { 0x13, "Invalid Parameter" },
  /* MIFARE */
  { 0x14, "Authentication Error" },
  /*  */
  { 0x23, "Wrong ISO/IEC14443-3 Check Byte" },
  { 0x25, "Invalid State" },
  { 0x26, "Operation Not Allowed" },
  { 0x27, "Command Not Acceptable" },
  { 0x29, "Target Released" },
  { 0x2a, "Card ID Mismatch" },
  { 0x2B, "Card Discarded" },
  { 0x2C, "NFCID3 Mismatch" },
  { 0x2D, "Over Current" },
  { 0x2E, "NAD Missing in DEP Frame" },

  /* Driver-level error */
  { DENACK,             "Received NACK" },
  { DEACKMISMATCH,      "Expected ACK/NACK" },
  { DEISERRFRAME,       "Received an error frame" },
  /* TODO: Move me in more generic code for libnfc 1.6 */
  { DEINVAL,            "Invalid argument" },
  { DEIO,               "Input/output error" },
  { DETIMEOUT,          "Operation timed-out" },
  { DENOTSUP,           "Operation not supported" }
};

const char *
pn53x_strerror (const nfc_device_t *pnd)
{
  const char *pcRes = "Unknown error";
  size_t i;

  for (i=0; i < (sizeof (sErrorMessages) / sizeof (struct sErrorMessage)); i++) {
    if (sErrorMessages[i].iErrorCode == pnd->iLastError) {
      pcRes = sErrorMessages[i].pcErrorMsg;
      break;
    }
  }

  return pcRes;
}

bool
pn53x_get_firmware_version (nfc_device_t *pnd)
{
  byte_t abtFw[4];
  size_t szFwLen = sizeof(abtFw);
  char* pcName;

  if (!pn53x_transceive(pnd,pncmd_get_firmware_version,2,abtFw,&szFwLen))
  {
    // Failed to get firmware revision??, whatever...let's disconnect and clean up and return err
    DBG("Failed to get firmware revision for: %s", pnd->acName);
    pnd->pdc->disconnect(pnd);
    return false;
  }

  // Add the firmware revision to the device name, PN531 gives 2 bytes info, but PN532 and PN533 gives 4
  pcName = strdup(pnd->acName);
  switch(pnd->nc) {
    case NC_PN531: snprintf(pnd->acName,DEVICE_NAME_LENGTH - 1,"%s - PN531 v%d.%d",pcName,abtFw[0],abtFw[1]); break;
    case NC_PN532: snprintf(pnd->acName,DEVICE_NAME_LENGTH - 1,"%s - PN532 v%d.%d (0x%02x)",pcName,abtFw[1],abtFw[2],abtFw[3]); break;
    case NC_PN533: snprintf(pnd->acName,DEVICE_NAME_LENGTH - 1,"%s - PN533 v%d.%d (0x%02x)",pcName,abtFw[1],abtFw[2],abtFw[3]); break;
  }
  free(pcName);
  return true;
}

bool
pn53x_configure(nfc_device_t* pnd, const nfc_device_option_t ndo, const bool bEnable)
{
  byte_t btValue;
  byte_t abtCmd[sizeof(pncmd_rf_configure)];

  pnd->iLastError = 0;

  memcpy(abtCmd,pncmd_rf_configure,sizeof(pncmd_rf_configure));

  // Make sure we are dealing with a active device
  if (!pnd->bActive) return false;

  switch(ndo)
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
      if (!pn53x_transceive(pnd,abtCmd,4,NULL,NULL)) return false;
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
      if (!pn53x_transceive(pnd,abtCmd,6,NULL,NULL)) return false;
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
    
    case NDO_AUTO_ISO14443_4:
      // TODO: PN53x parameters could not be read, so we have to buffered current value in order to prevent from configuration overwrite
      // ATM, buffered current value is not needed due to a single usage of these parameters
      btValue = (bEnable) ? (SYMBOL_PARAM_fAutomaticRATS | SYMBOL_PARAM_fAutomaticATR_RES): SYMBOL_PARAM_fAutomaticATR_RES;
      if(!pn53x_set_parameters(pnd,btValue)) return false;
      return true;
    break;
  }

  // When we reach this, the configuration is completed and succesful
  return true;
}

bool pn53x_select_dep_target(nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t* pnti)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t offset;
  byte_t abtCmd[sizeof(pncmd_initiator_jump_for_dep)];

  pnd->iLastError = 0;

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

bool pn53x_transceive_dep_bytes(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_data)];

  pnd->iLastError = 0;

  memcpy(abtCmd,pncmd_initiator_exchange_data,sizeof(pncmd_initiator_exchange_data));

  // We can not just send bytes without parity if while the PN53X expects we handled them
  if (!pnd->bPar) return false;

  // Copy the data into the command frame
  abtCmd[2] = 1; /* target number */
  memcpy(abtCmd+3,pbtTx,szTxLen);

  // To transfer command frames bytes we can not have any leading bits, reset this to zero
  if (!pn53x_set_tx_bits(pnd,0)) return false;

  // Send the frame to the PN53X chip and get the answer
  // We have to give the amount of bytes + (the two command bytes 0xD4, 0x40)
  if (!pn53x_transceive(pnd,abtCmd,szTxLen+3,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everything went successful
  return true;
}

bool pn53x_transceive_bits(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  size_t szFrameBits = 0;
  size_t szFrameBytes = 0;
  uint8_t ui8Bits = 0;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];

  pnd->iLastError = 0;

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

bool pn53x_transceive_bytes(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  byte_t abtCmd[sizeof(pncmd_initiator_exchange_raw_data)];

  pnd->iLastError = 0;

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

bool pn53x_target_receive_dep_bytes(nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;

  pnd->iLastError = 0;


  // Try to gather a received frame from the reader
  if (!pn53x_transceive(pnd,pncmd_target_get_data,2,abtRx,&szRxLen)) return false;

  // Save the received byte count
  *pszRxLen = szRxLen-1;

  // Copy the received bytes
  memcpy(pbtRx,abtRx+1,*pszRxLen);

  // Everyting seems ok, return true
  return true;
}

bool pn53x_target_send_dep_bytes(nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen)
{
  byte_t abtCmd[sizeof(pncmd_target_set_data)];

  pnd->iLastError = 0;

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

bool pn53x_target_init(nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits)
{
  byte_t abtRx[MAX_FRAME_LEN];
  size_t szRxLen;
  uint8_t ui8Bits;
  // Save the current configuration settings
  bool bCrc = pnd->bCrc;
  bool bPar = pnd->bPar;
  byte_t abtCmd[sizeof(pncmd_target_init)];

  pnd->iLastError = 0;

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

  // Request the initialization as a target
  szRxLen = MAX_FRAME_LEN;
  if (!pn53x_transceive(pnd,abtCmd,39,abtRx,&szRxLen)) return false;

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
