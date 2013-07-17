/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
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
 * @file nfc-emulation.h
 * @brief Provide a small API to ease emulation in libnfc
 */

#ifndef __NFC_EMULATION_H__
#define __NFC_EMULATION_H__

#include <sys/types.h>
#include <nfc/nfc.h>

#ifdef __cplusplus
extern  "C" {
#endif /* __cplusplus */

struct nfc_emulator;
struct nfc_emulation_state_machine;

/**
 * @struct nfc_emulator
 * @brief NFC emulator structure
 */
struct nfc_emulator {
  nfc_target *target;
  struct nfc_emulation_state_machine *state_machine;
  void *user_data;
};

/**
 * @struct nfc_emulation_state_machine
 * @brief  NFC emulation state machine structure
 */
struct nfc_emulation_state_machine {
  int (*io)(struct nfc_emulator *emulator, const uint8_t *data_in, const size_t data_in_len, uint8_t *data_out, const size_t data_out_len);
  void *data;
};

NFC_EXPORT int    nfc_emulate_target(nfc_device *pnd, struct nfc_emulator *emulator, const int timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __NFC_EMULATION_H__ */
