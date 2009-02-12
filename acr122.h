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

#ifndef _LIBNFC_ACR122_H_
#define _LIBNFC_ACR122_H_

#include "defines.h"
#include "types.h"

dev_id acr122_connect(const ui32 uiDeviceIndex);
void acr122_disconnect(const dev_id di);
bool acr122_transceive(const dev_id di, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen);
bool acr122_transceive_(const dev_id di, const byte* pbtTx, const ui32 uiTxLen, byte* pbtRx, ui32* puiRxLen);

char* acr122_firmware(const dev_id di);
bool acr122_led_red(const dev_id di, bool bOn);

#endif // _LIBNFC_ACR122_H_

