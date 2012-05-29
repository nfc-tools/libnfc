/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
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
 * @file drivers.h
 * @brief Supported drivers header
 */

#ifndef __NFC_DRIVERS_H__
#  define __NFC_DRIVERS_H__

#  include <nfc/nfc-types.h>

#  if defined (DRIVER_ACR122_PCSC_ENABLED)
#    include "drivers/acr122_pcsc.h"
#  endif /* DRIVER_ACR122_PCSC_ENABLED */

#  if defined (DRIVER_ACR122_USB_ENABLED)
#    include "drivers/acr122_usb.h"
#  endif /* DRIVER_ACR122_USB_ENABLED */

#  if defined (DRIVER_ACR122S_ENABLED)
#    include "drivers/acr122s.h"
#  endif /* DRIVER_ACR122S_ENABLED */

#  if defined (DRIVER_PN53X_USB_ENABLED)
#    include "drivers/pn53x_usb.h"
#  endif /* DRIVER_PN53X_USB_ENABLED */

#  if defined (DRIVER_ARYGON_ENABLED)
#    include "drivers/arygon.h"
#  endif /* DRIVER_ARYGON_ENABLED */

#  if defined (DRIVER_PN532_UART_ENABLED)
#    include "drivers/pn532_uart.h"
#  endif /* DRIVER_PN532_UART_ENABLED */

#  define DRIVERS_MAX_DEVICES         16

extern const struct nfc_driver *nfc_drivers[];

#endif // __NFC_DRIVERS_H__
