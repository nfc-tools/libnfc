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

 /**
 * @file nfc-emulate.c
 * @brief Provide a small API to ease emulation in libnfc
 */

#include <nfc/nfc.h>
#include <nfc/nfc-emulation.h>

#include "iso7816.h"

int
nfc_emulate_target (nfc_device_t* pnd, struct nfc_emulator *emulator)
{
  byte_t abtRx[ISO7816_SHORT_R_APDU_MAX_LEN];
  size_t szRx = sizeof(abtRx);
  byte_t abtTx[ISO7816_SHORT_C_APDU_MAX_LEN];
  int res = 0;

  if (!nfc_target_init (pnd, emulator->target, abtRx, &szRx)) {
    return -1;
  }

  while (res >= 0) {
    res = emulator->state_machine->io (emulator, abtRx, szRx, abtTx, sizeof (abtTx));
    if (res > 0) {
      if (!nfc_target_send_bytes(pnd, abtTx, res)) {
        return -1;
      }
    }
    if (res >= 0) {
      if (!nfc_target_receive_bytes(pnd, abtRx, &szRx)) {
        return -1;
      }
    }
  }
  return (res < 0) ? res : 0;
}
