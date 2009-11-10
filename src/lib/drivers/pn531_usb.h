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
 * @file pn531_usb.h
 * @brief
 */

#ifndef _LIBNFC_DEV_PN531_H_
#define _LIBNFC_DEV_PN531_H_

#include <stdint.h>
#include <stdbool.h>


#include "nfc-types.h"

// Functions used by developer to handle connection to this device
nfc_device_t* pn531_usb_connect(const nfc_device_desc_t* pndd);
void pn531_usb_disconnect(nfc_device_t* pnd);

// Callback function used by libnfc to transmit commands to the PN53X chip
bool pn531_usb_transceive(const dev_spec ds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);

#endif // _LIBNFC_DEV_PN531_H_

