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
* @file nfc-emulation.c
* @brief Provide a small API to ease emulation in libnfc
*/

#include <nfc/nfc.h>
#include <nfc/nfc-emulation.h>

#include "iso7816.h"

int
nfc_emulate_target(nfc_device *pnd, struct nfc_emulator *emulator)
{
  uint8_t abtRx[ISO7816_SHORT_R_APDU_MAX_LEN];
  uint8_t abtTx[ISO7816_SHORT_C_APDU_MAX_LEN];

  int res;
  if ((res = nfc_target_init(pnd, emulator->target, abtRx, sizeof(abtRx), 0)) < 0) {
    return res;
  }

  size_t szRx = res;
  int io_res = res;
  while (io_res >= 0) {
    io_res = emulator->state_machine->io(emulator, abtRx, szRx, abtTx, sizeof(abtTx));
    if (io_res > 0) {
      if ((res = nfc_target_send_bytes(pnd, abtTx, io_res, 0)) < 0) {
        return res;
      }
    }
    if (io_res >= 0) {
      if ((res = nfc_target_receive_bytes(pnd, abtRx, sizeof(abtRx), 0)) < 0) {
        return res;
      }
      szRx = res;
    }
  }
  return (io_res < 0) ? io_res : 0;
}
