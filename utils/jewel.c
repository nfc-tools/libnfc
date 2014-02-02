/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2014      Pim 't Hart
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
 * @file jewel.c
 * @brief provide samples structs and functions to manipulate Jewel Topaz tags using libnfc
 */
#include "jewel.h"

#include <string.h>

#include <nfc/nfc.h>

/**
 * @brief Execute a Jewel Topaz Command
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param req The request
 * @param res The response
 *
 */

bool
nfc_initiator_jewel_cmd(nfc_device *pnd, const jewel_req req, jewel_res *pres)
{
  size_t nLenReq;
  size_t nLenRes;

  switch (req.rid.btCmd) {
    case TC_RID:
      nLenReq = sizeof(jewel_req_rid);
      nLenRes = sizeof(jewel_res_rid);
      break;
    case TC_RALL:
      nLenReq = sizeof(jewel_req_rall);
      nLenRes = sizeof(jewel_res_rall);
      break;
    case TC_READ:
      nLenReq = sizeof(jewel_req_read);
      nLenRes = sizeof(jewel_res_read);
      break;
    case TC_WRITEE:
      nLenReq = sizeof(jewel_req_writee);
      nLenRes = sizeof(jewel_res_writee);
      break;
    case TC_WRITENE:
      nLenReq = sizeof(jewel_req_writene);
      nLenRes = sizeof(jewel_res_writene);
      break;
    case TC_RSEG:
      nLenReq = sizeof(jewel_req_rseg);
      nLenRes = sizeof(jewel_res_rseg);
      break;
    case TC_READ8:
      nLenReq = sizeof(jewel_req_read8);
      nLenRes = sizeof(jewel_res_read8);
      break;
    case TC_WRITEE8:
      nLenReq = sizeof(jewel_req_writee8);
      nLenRes = sizeof(jewel_res_writee8);
      break;
    case TC_WRITENE8:
      nLenReq = sizeof(jewel_req_writene8);
      nLenRes = sizeof(jewel_res_writene8);
      break;
    default:
      return false;
      break;
  }

  if (nfc_initiator_transceive_bytes(pnd, (uint8_t *)&req, nLenReq, (uint8_t *)pres, nLenRes, -1) < 0) {
    nfc_perror(pnd, "nfc_initiator_transceive_bytes");
    return false;
  }

  return true;
}
