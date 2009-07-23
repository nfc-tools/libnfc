/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#ifndef _LIBNFC_H_
#define _LIBNFC_H_

#include <stdint.h>
#include <stdbool.h>

#include "defines.h"
#include "types.h"
#include "bitutils.h"

dev_info* nfc_connect(void);
void nfc_disconnect(dev_info* pdi);
bool nfc_configure(dev_info* pdi, const dev_config_option dco, const bool bEnable);

bool nfc_initiator_init(const dev_info* pdi);
bool nfc_initiator_select_tag(const dev_info* pdi, const init_modulation im, const byte_t* pbtInitData, const uint32_t uiInitDataLen, tag_info* pti);
bool nfc_initiator_deselect_tag(const dev_info* pdi);
bool nfc_initiator_transceive_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar);
bool nfc_initiator_transceive_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen);
bool nfc_initiator_mifare_cmd(const dev_info* pdi, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp);

bool nfc_target_init(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits);
bool nfc_target_receive_bits(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar);
bool nfc_target_receive_bytes(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxLen);
bool nfc_target_send_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar);
bool nfc_target_send_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen);

#endif // _LIBNFC_H_

