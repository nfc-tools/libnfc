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
 * @file pn53x_usb.h
 * @brief Drive for PN53x USB devices
 */

#ifndef __NFC_DRIVER_PN53X_USB_H__
#  define __NFC_DRIVER_PN53X_USB_H__

#  include <nfc/nfc-types.h>

bool    pn53x_usb_probe (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
nfc_device_t *pn53x_usb_connect (const nfc_device_desc_t * pndd);
bool    pn53x_usb_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData);
int     pn53x_usb_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szData);
void    pn53x_usb_disconnect (nfc_device_t * pnd);

extern const struct nfc_driver_t pn53x_usb_driver;

#endif // ! __NFC_DRIVER_PN53X_USB_H__
