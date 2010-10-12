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
 * @file arygon.h
 * @brief
 */

#ifndef __NFC_DRIVER_ARYGON_H__
#  define __NFC_DRIVER_ARYGON_H__

#  include <nfc/nfc-types.h>

#  define ARYGON_DRIVER_NAME "ARYGON"

// Functions used by developer to handle connection to this device
nfc_device_desc_t *arygon_pick_device (void);
bool    arygon_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);

nfc_device_t *arygon_connect (const nfc_device_desc_t * pndd);
void    arygon_disconnect (nfc_device_t * pnd);

// Callback function used by libnfc to transmit commands to the PN53X chip
bool    arygon_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                           size_t * pszRx);

#endif // ! __NFC_DRIVER_ARYGON_H__
