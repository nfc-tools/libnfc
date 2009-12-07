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
 * @brief
 */

#include <sys/param.h>
#include <stdio.h>
#include <stddef.h>

#include <usb.h>

typedef struct {
  usb_dev_handle* pudh;
  uint32_t uiEndPointIn;
  uint32_t uiEndPointOut;
} usb_spec_t;

nfc_device_t* pn53x_usb_connect(const nfc_device_desc_t* pndd, char * target_name, int target_chip);
void get_end_points(struct usb_device *dev, usb_spec_t* pus);
void pn53x_usb_disconnect(nfc_device_t* pnd);
bool pn53x_usb_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);
bool pn53x_usb_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound,int idvendor, int idproduct, char * target_name);
