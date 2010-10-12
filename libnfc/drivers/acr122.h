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
 * @file acr122.h
 * @brief
 */

#ifndef __NFC_DRIVER_ACR122_H__
#  define __NFC_DRIVER_ACR122_H__

#  include <stdint.h>
#  include <stdbool.h>


#  include <nfc/nfc-types.h>

#  define ACR122_DRIVER_NAME "ACR122"

nfc_device_desc_t *acr122_pick_device (void);
bool    acr122_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);

// Functions used by developer to handle connection to this device
nfc_device_t *acr122_connect (const nfc_device_desc_t * pndd);
void    acr122_disconnect (nfc_device_t * pnd);

// Callback function used by libnfc to transmit commands to the PN53X chip
bool    acr122_transceive (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                           size_t * pszRx);

// Various additional features this device supports
char   *acr122_firmware (const nfc_device_spec_t nds);
bool    acr122_led_red (const nfc_device_spec_t nds, bool bOn);

#endif // ! __NFC_DRIVER_ACR122_H__
