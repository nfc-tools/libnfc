/**
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2011, Romain Tartiere, Romuald Conty
 * Copyright (C) 2012, Romuald Conty
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
 * @file acr122_pcsc.h
 * @brief Driver for ACR122 devices (behind PC/SC daemon)
 */

#ifndef __NFC_DRIVER_ACR122_PCSC_H__
#  define __NFC_DRIVER_ACR122_PCSC_H__

#  include <nfc/nfc-types.h>

bool    acr122_pcsc_probe(nfc_connstring connstrings[], size_t connstrings_len, size_t *pszDeviceFound);

// Functions used by developer to handle connection to this device
nfc_device *acr122_pcsc_open(const nfc_connstring connstring);
int    acr122_pcsc_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout);
int     acr122_pcsc_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szData, int timeout);
void    acr122_pcsc_close(nfc_device *pnd);

extern const struct nfc_driver acr122_pcsc_driver;

#endif // ! __NFC_DRIVER_ACR122_PCSC_H__
