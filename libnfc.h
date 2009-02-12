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
#include "acr122.h"

bool nfc_configure_handle_crc(const dev_id di, const bool bEnable);
bool nfc_configure_handle_parity(const dev_id di, const bool bEnable);
bool nfc_configure_field(const dev_id di, const bool bEnable);
bool nfc_configure_list_passive_infinite(const dev_id di, const bool bEnable);
bool nfc_configure_accept_invalid_frames(const dev_id di, const bool bEnable);
bool nfc_configure_accept_multiple_frames(const dev_id di, const bool bEnable);

bool nfc_reader_init(const dev_id di);
bool nfc_reader_list_passive(const dev_id di, const ModulationType mt, const byte* pbtInitData, const ui32 uiInitDataLen, byte* pbtTag, ui32* puiTagLen);
bool nfc_reader_transceive_7bits(const dev_id di, const byte btTx, byte* pbtRx, ui32* puiRxLen);
bool nfc_reader_transceive_bytes(const dev_id di, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen);
bool nfc_reader_transceive_bits(const dev_id di, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar);
bool nfc_reader_mifare_cmd(const dev_id di, const MifareCmd mc, const ui8 ui8Block, MifareParam* pmp);

bool nfc_target_init(const dev_id di, byte* pbtRx, ui32* puiRxLen);
bool nfc_target_receive_bytes(const dev_id di, byte* pbtRx, ui32* puiRxLen);
bool nfc_target_receive_bits(const dev_id di, byte* pbtRx, ui32* puiRxBits, byte* pbtRxPar);
bool nfc_target_send_bytes(const dev_id di, const byte* pbtTx, const ui32 uiTxLen);
bool nfc_target_send_bits(const dev_id di, const byte* pbtTx, const ui32 uiTxBits, const byte* pbtTxPar);

#endif // _LIBNFC_H_

