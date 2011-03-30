/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2011, Romain Tartière, Romuald Conty
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
 * @file nfc-device.c
 * @brief Provide internal function to manipulate nfc_device_t type
 */

/* vim:set et sw=2 ts=2: */

#include <stdlib.h>

#include "nfc-internal.h"

nfc_device_t *
nfc_device_new (void)
{
  nfc_device_t *res = malloc (sizeof (*res));

  if (!res) {
    err (EXIT_FAILURE, "nfc_device_new: malloc");
  }

  res->bCrc = true;
  res->bPar = true;
  res->bEasyFraming    = true;
  res->bAutoIso14443_4 = true;
  res->iLastError  = 0;
  res->driver_data = NULL;
  res->chip_data   = NULL;

  return res;
}

void
nfc_device_free (nfc_device_t *nfc_device)
{
  if (nfc_device) {
    free (nfc_device->driver_data);
    free (nfc_device->chip_data);
    free (nfc_device);
  }
}
