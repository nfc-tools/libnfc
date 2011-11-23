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

bool acr122s_probe(nfc_device_desc_t descs[], size_t desc_count, size_t *dev_found);

nfc_device_t *acr122s_connect(const nfc_device_desc_t *desc);
void acr122s_disconnect(nfc_device_t *dev);

bool acr122s_send(nfc_device_t *dev, const byte_t *buf, size_t buf_len, struct timeval *timeout);
int acr122s_receive(nfc_device_t *dev, byte_t *buf, size_t buf_len, struct timeval *timeout);

extern const struct nfc_driver_t acr122s_driver;

#endif
