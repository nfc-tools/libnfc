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
 * @file pn533_usb.c
 * @brief Driver for PN533 chip using USB
 */

/*
Thanks to d18c7db and Okko for example code
*/

#include "../drivers.h"
#include <nfc/nfc-messages.h>

nfc_device_desc_t * pn533_usb_pick_device (void)
{
  nfc_device_desc_t *pndd;

  if ((pndd = malloc (sizeof (*pndd)))) {
    size_t szN;

    if (!pn533_usb_list_devices (pndd, 1, &szN)) {
      DBG("%s", "pn533_usb_list_devices failed");
      return NULL;
    }

    if (szN == 0) {
      ERR("%s", "No device found");
      return NULL;
    }
  }

  return pndd;
}

bool pn533_usb_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
{
  int idvendor = 0x04cc;
  int idproduct = 0x2533;
  int idvendor_alt = 0x04e6;
  int idproduct_alt = 0x5591;

  size_t firstpass = 0;
  
  pn53x_usb_list_devices(&pnddDevices[0], szDevices, pszDeviceFound, idvendor, idproduct, PN533_USB_DRIVER_NAME);
  if(*pszDeviceFound == szDevices)
    return true;
  firstpass= *pszDeviceFound;
  pn53x_usb_list_devices(&pnddDevices[firstpass], szDevices, pszDeviceFound, idvendor_alt, idproduct_alt, PN533_USB_DRIVER_NAME);
  (*pszDeviceFound) += firstpass;

  if(*pszDeviceFound) 
    return true;
  return false;
}

nfc_device_t* pn533_usb_connect(const nfc_device_desc_t* pndd)
{
  return(pn53x_usb_connect(pndd, PN533_USB_DRIVER_NAME, NC_PN533));
}
