/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
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


struct nfc_emulator {
  nfc_target_t *target;
  struct nfc_emulation_state_machine *state_machine;
  void *user_data;
};

struct nfc_emulation_state_machine {
  int (*io)(struct nfc_emulator *emulator, const byte_t *data_in, const size_t data_in_len, byte_t *data_out, const size_t data_out_len);
  void *data;
};

NFC_EXPORT int    nfc_emulate_target (nfc_device_t* pnd, struct nfc_emulator *emulator);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __NFC_EMULATION_H__ */
