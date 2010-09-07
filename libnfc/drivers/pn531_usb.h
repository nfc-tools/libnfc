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

#ifndef __NFC_DRIVER_PN531_USB_H__
#  define __NFC_DRIVER_PN531_USB_H__

#  define PN531_USB_DRIVER_NAME "PN531_USB"

// Functions used by developer to handle connection to this device
nfc_device_t *pn531_usb_connect (const nfc_device_desc_t * pndd);
bool    pn531_usb_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);
nfc_device_desc_t *pn531_usb_pick_device (void);

#endif // ! __NFC_DRIVER_PN531_USB_H__
