/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
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
 * @file nfc-emulation.c
 * @brief Provide a small API to ease emulation in libnfc
 */

#include <nfc/nfc.h>
#include <nfc/nfc-emulation.h>

#include "iso7816.h"

/** @ingroup emulation
 * @brief Emulate a target
 * @return Returns 0 on success, otherwise returns libnfc's error code (negative value).
 *
 * @param pnd \a nfc_device struct pointer that represents currently used device
 * @param emulator \nfc_emulator struct point that handles input/output functions
 *
 * If timeout equals to 0, the function blocks indefinitely (until an error is raised or function is completed)
 * If timeout equals to -1, the default timeout will be used
 */
int
nfc_emulate_target(nfc_device *pnd, struct nfc_emulator *emulator, const int timeout)
{
  uint8_t abtRx[ISO7816_SHORT_R_APDU_MAX_LEN];
  uint8_t abtTx[ISO7816_SHORT_C_APDU_MAX_LEN];

  int res;
  if ((res = nfc_target_init(pnd, emulator->target, abtRx, sizeof(abtRx), timeout)) < 0) {
    return res;
  }

  size_t szRx = res;
  int io_res = res;
  while (io_res >= 0) {
    io_res = emulator->state_machine->io(emulator, abtRx, szRx, abtTx, sizeof(abtTx));
    if (io_res > 0) {
      if ((res = nfc_target_send_bytes(pnd, abtTx, io_res, timeout)) < 0) {
        return res;
      }
    }
    if (io_res >= 0) {
      if ((res = nfc_target_receive_bytes(pnd, abtRx, sizeof(abtRx), timeout)) < 0) {
        return res;
      }
      szRx = res;
    }
  }
  return (io_res < 0) ? io_res : 0;
}

