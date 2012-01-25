/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2011, Anugrah Redja Kusuma <anugrah.redja@gmail.com>
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
 */

/**
 * @file acr122s.h
 * @brief Driver for ACS ACR122S devices
 */

#ifndef __NFC_DRIVER_ACR122S_H__
#define __NFC_DRIVER_ACR122S_H__

#include <sys/time.h>
#include <nfc/nfc-types.h>

bool acr122s_probe(nfc_connstring connstrings[], size_t connstrings_len, size_t *pszDeviceFound);

nfc_device *acr122s_open(const nfc_connstring connstring);
void acr122s_close(nfc_device *pnd);

int acr122s_send(nfc_device *pnd, const uint8_t *buf, size_t buf_len, int timeout);
int acr122s_receive(nfc_device *pnd, uint8_t *buf, size_t buf_len, int timeout);

extern const struct nfc_driver acr122s_driver;

#endif
