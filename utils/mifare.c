/*-
 * Public platform independent Near Field Communication (NFC) library examples
 *
 * Copyright (C) 2009 Roel Verdult
 * Copyright (C) 2010 Romain Tartière
 * Copyright (C) 2010, 2011 Romuald Conty
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */
/**
 * @file mifare.c
 * @brief provide samples structs and functions to manipulate MIFARE Classic and Ultralight tags using libnfc
 */
#include "mifare.h"

#include <string.h>

#include <nfc/nfc.h>

/**
 * @brief Execute a MIFARE Classic Command
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pmp Some commands need additional information. This information should be supplied in the mifare_param union.
 *
 * The specified MIFARE command will be executed on the tag. There are different commands possible, they all require the destination block number.
 * @note There are three different types of information (Authenticate, Data and Value).
 *
 * First an authentication must take place using Key A or B. It requires a 48 bit Key (6 bytes) and the UID.
 * They are both used to initialize the internal cipher-state of the PN53X chip (http://libnfc.org/hardware/pn53x-chip).
 * After a successful authentication it will be possible to execute other commands (e.g. Read/Write).
 * The MIFARE Classic Specification (http://www.nxp.com/acrobat/other/identification/M001053_MF1ICS50_rev5_3.pdf) explains more about this process.
 */
bool
nfc_initiator_mifare_cmd(nfc_device *pnd, const mifare_cmd mc, const uint8_t ui8Block, mifare_param *pmp)
{
  uint8_t  abtRx[265];
  size_t  szParamLen;
  uint8_t  abtCmd[265];
  //bool    bEasyFraming;

  abtCmd[0] = mc;               // The MIFARE Classic command
  abtCmd[1] = ui8Block;         // The block address (1K=0x00..0x39, 4K=0x00..0xff)

  switch (mc) {
      // Read and store command have no parameter
    case MC_READ:
    case MC_STORE:
      szParamLen = 0;
      break;

      // Authenticate command
    case MC_AUTH_A:
    case MC_AUTH_B:
      szParamLen = sizeof(struct mifare_param_auth);
      break;

      // Data command
    case MC_WRITE:
      szParamLen = sizeof(struct mifare_param_data);
      break;

      // Value command
    case MC_DECREMENT:
    case MC_INCREMENT:
    case MC_TRANSFER:
      szParamLen = sizeof(struct mifare_param_value);
      break;

      // Please fix your code, you never should reach this statement
    default:
      return false;
      break;
  }

  // When available, copy the parameter bytes
  if (szParamLen)
    memcpy(abtCmd + 2, (uint8_t *) pmp, szParamLen);

  // FIXME: Save and restore bEasyFraming
  // bEasyFraming = nfc_device_get_property_bool (pnd, NP_EASY_FRAMING, &bEasyFraming);
  if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true) < 0) {
    nfc_perror(pnd, "nfc_device_set_property_bool");
    return false;
  }
  // Fire the mifare command
  int res;
  if ((res = nfc_initiator_transceive_bytes(pnd, abtCmd, 2 + szParamLen, abtRx, sizeof(abtRx), -1))  < 0) {
    if (res == NFC_ERFTRANS) {
      // "Invalid received frame",  usual means we are
      // authenticated on a sector but the requested MIFARE cmd (read, write)
      // is not permitted by current acces bytes;
      // So there is nothing to do here.
    } else {
      nfc_perror(pnd, "nfc_initiator_transceive_bytes");
    }
    // XXX nfc_device_set_property_bool (pnd, NP_EASY_FRAMING, bEasyFraming);
    return false;
  }
  /* XXX
  if (nfc_device_set_property_bool (pnd, NP_EASY_FRAMING, bEasyFraming) < 0) {
    nfc_perror (pnd, "nfc_device_set_property_bool");
    return false;
  }
  */

  // When we have executed a read command, copy the received bytes into the param
  if (mc == MC_READ) {
    if (res == 16) {
      memcpy(pmp->mpd.abtData, abtRx, 16);
    } else {
      return false;
    }
  }
  // Command succesfully executed
  return true;
}
