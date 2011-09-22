/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2010, Roel Verdult, Romuald Conty
 * Copyright (C) 2011, Romuald Conty, Romain Tartière
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
 * @file pn532_uart.h
 * @brief Driver for PN532 connected in UART (HSU)
 */

#ifndef __NFC_DRIVER_PN532_UART_H__
#  define __NFC_DRIVER_PN532_UART_H__

#  include <sys/time.h>

#  include <nfc/nfc-types.h>

bool    pn532_uart_probe (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound);

nfc_device_t *pn532_uart_connect (const nfc_device_desc_t * pndd);
void    pn532_uart_disconnect (nfc_device_t * pnd);
bool    pn532_uart_send (nfc_device_t * pnd, const byte_t * pbtData, const size_t szData, struct timeval *timeout);
int     pn532_uart_receive (nfc_device_t * pnd, byte_t * pbtData, const size_t szData, struct timeval *timeout);

extern const struct nfc_driver_t pn532_uart_driver;

#endif // ! __NFC_DRIVER_PN532_UART_H__
