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
