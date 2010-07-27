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

#include <string.h>
#include <stdio.h>

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

const char *
pn53x_err2string (int iError, char const **ppcDescription);

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
  printf ("Entering transceive (bsin = %lu, bsout = %lu)\n", szTxLen, *pszRxLen);
  if (!pnd->pdc->transceive(pnd->nds,pbtTx,szTxLen,pbtRx,pszRxLen)) return false;
  printf ("Leaving transceive (bsin = %lu, bsout = %lu)\n", szTxLen, *pszRxLen);

  pnd->iErrorCode = pbtRx[0] & 0x3f;

  // Make sure there was no failure reported by the PN53X chip (0x00 == OK)
  if (pnd->iErrorCode != 0) {
    const char *s, *l;

    s = pn53x_err2string (pnd->iErrorCode, &l);
    if (s) {
      printf (" s = %s\n l = %s\n", s, l);
    }
  }

  // Succesful transmission
  return (0 == pnd->iErrorCode);
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

bool 
pn53x_decode_target_data(const byte_t* pbtRawData, size_t szDataLen, nfc_chip_t nc, nfc_target_type_t ntt, nfc_target_info_t* pnti)
{
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
      break;
    default:
      return false;
      break;
  }
  return true;
}

bool
pn53x_InListPassiveTarget(const nfc_device_t* pnd,
                          const nfc_modulation_t nmInitModulation, const byte_t szMaxTargets,
                          const byte_t* pbtInitiatorData, const size_t szInitiatorDataLen,
                          byte_t* pbtTargetsData, size_t* pszTargetsData)
{
  byte_t abtCmd[sizeof(pncmd_initiator_list_passive)];
  memcpy(abtCmd,pncmd_initiator_list_passive,sizeof(pncmd_initiator_list_passive));

  abtCmd[2] = szMaxTargets;  // MaxTg
  abtCmd[3] = nmInitModulation; // BrTy, the type of init modulation used for polling a passive tag

  // Set the optional initiator data (used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID).
  if (pbtInitiatorData) memcpy(abtCmd+4,pbtInitiatorData,szInitiatorDataLen);

  // Try to find a tag, call the tranceive callback function of the current device
  size_t szRxLen = MAX_FRAME_LEN;
  // We can not use pn53x_transceive() because abtRx[0] gives no status info
  if(pnd->pdc->transceive(pnd->nds,abtCmd,4+szInitiatorDataLen,pbtTargetsData,&szRxLen)) {
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

struct sErrorMessage {
  int iError;
  const char *pcErrorMsg;
  const char *pcErrorDescription;
} sErrorMessages[] = {
  { 0x00, "success",                "The request was successful." },
  { 0x01, "timout",                 "Time Out, the target has not answered." },
  { 0x02, "crc error",              "A CRC error has been detected by the CIU." },
  { 0x03, "parity error",           "A Parity error has been detected by the CIU." },
  { 0x04, "wrong bit count",        "During an anti-collision/select operation (ISO/IEC14443-3 Type A and ISO/"
                                    "IEC18092 106 kbps passive mode), an erroneous Bit Count has been detected." },
  { 0x05, "framing error",          "Framing error during Mifare operation." },
  { 0x06, "bit-collision",          "An abnormal bit-collision has been detected during bit wise anti-collision"
                                    " at 106 kbps." },
  { 0x07, "buffer too small",       "Communication buffer size insufficient." },
  { 0x09, "buffer overflow",        "RF Buffer overflow has been detected by the CIU (bit BufferOvfl of the register"
                                    " CIU_Error)." },
  { 0x0a, "timout",                 "In active communication mode, the RF field has not been switched on in time by"
                                    " the counterpart (as defined in NFCIP-1 standard)." },
  { 0x0b, "protocol error",         "RF Protocol error." },
  { 0x0d, "temerature",             "The internal temperature sensor has detected overheating, and therefore has"
                                    " automatically switched off the antenna drivers." },
  { 0x0e, "overflow",               "Internal buffer overflow." },
  { 0x10, "invalid parameter",      "Invalid parameter."},
  /* DEP Errors */
  { 0x12, "unknown command",        "The PN532 configured in target mode does not support the command received from"
                                    " the initiator." },
  { 0x13, "invalid parameter",      "The data format does not match to the specification." },
  /* MIFARE */
  { 0x14, "authentication",         "Authentication error." },
  { 0x23, "check byte",             "ISO/IEC14443-3: UID Check byte is wrong." },
  { 0x25, "invalid state",          "The system is in a state which does not allow the operation." },
  { 0x26, "operation not allowed",  "Operation not allowed in this configuration (host controller interface)." },
  { 0x27, "command not acceptable", "This command is not acceptable due to the current context of the PN532"
                                    " (Initiator vs. Target, unknown target number, Target not in the good state,"
                                    " ...)" },
  { 0x29, "target released",        "The PN532 configured as target has been released by its initiator." },
  { 0x2a, "card id mismatch",       "PN532 and ISO/IEC14443-3B only: the ID of the card does not match, meaning "
                                    "that the expected card has been exchanged with another one." },
  { 0x2B, "card discarded",         "PN532 and ISO/IEC14443-3B only: the card previously activated has disappeared." },
  { 0x2C, "NFCID3 mismatch",        "Mismatch between the NFCID3 initiator and the NFCID3 target in DEP 212/424 kbps"
                                    " passive." },
  { 0x2D, "over current",           "An over-current event has been detected." },
  { 0x2E, "NAD missing",            "NAD missing in DEP frame." },
};

const char *
pn53x_err2string (int iError, char const **ppcDescription)
{
  const char *pcRes = NULL;

  for (size_t i=0; i < (sizeof (sErrorMessages) / sizeof (struct sErrorMessage)); i++) {
    if (sErrorMessages[i].iError == iError) {
      pcRes = sErrorMessages[i].pcErrorMsg;
      if (ppcDescription)
        *ppcDescription = sErrorMessages[i].pcErrorDescription;
      break;
    }
  }

  return pcRes;
}

