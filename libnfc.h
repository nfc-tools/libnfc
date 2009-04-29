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

#ifndef _LIBNFC_H_
#define _LIBNFC_H_

#include "defines.h"
#include "types.h"
#include "bitutils.h"
#include "devices.h"

dev_info* nfc_connect();
void nfc_disconnect(dev_info* pdi);
bool nfc_configure(dev_info* pdi, const dev_config_option dco, const bool bEnable);

bool nfc_reader_init(const dev_info* pdi);
bool nfc_reader_select(const dev_info* pdi, const init_modulation im, const byte* pbtInitData, const ui32 uiInitDataLen, tag_info* pti);
bool nfc_reader_deselect(const dev_info* pdi);
bool nfc_reader_transceive_bits(const dev_info* pdi, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar);
bool nfc_reader_transceive_bytes(const dev_info* pdi, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen);
bool nfc_reader_mifare_cmd(const dev_info* pdi, const mifare_cmd mc, const ui8 ui8Block, mifare_param* pmp);

bool nfc_target_init(const dev_info* pdi, byte* pbtRx, ui32* puiRxBits);
bool nfc_target_receive_bits(const dev_info* pdi, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar);
bool nfc_target_receive_bytes(const dev_info* pdi, byte* pbtRx, ui32* puiRxLen);
bool nfc_target_send_bits(const dev_info* pdi, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar);
bool nfc_target_send_bytes(const dev_info* pdi, const byte* pbtTx, const ui32 uiTxLen);

#endif // _LIBNFC_H_

