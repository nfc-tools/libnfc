/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * 
 * 
 * @file drivers.h
 * @brief
 */

#ifndef __NFC_DRIVERS_H__
#define __NFC_DRIVERS_H__

#include <nfc/nfc-types.h>

#ifdef HAVE_PCSC_LITE
  #include "drivers/acr122.h"
#endif /* HAVE_PCSC_LITE */

#ifdef HAVE_LIBUSB
  #include "drivers/pn531_usb.h"
  #include "drivers/pn533_usb.h"
#endif /* HAVE_LIBUSB */

#include "drivers/arygon.h"
#include "drivers/pn532_uart.h"

#define DRIVERS_MAX_DEVICES         16
#define MAX_FRAME_LEN       264

const static struct driver_callbacks drivers_callbacks_list[] = {
//  Driver Name             Pick Device    List Devices        Connect                  Transceive                    Disconnect
#ifdef HAVE_PCSC_LITE
  { ACR122_DRIVER_NAME,     acr122_pick_device,  acr122_list_devices, acr122_connect,      acr122_transceive,        acr122_disconnect       },
#endif /* HAVE_PCSC_LITE */
#ifdef HAVE_LIBUSB
  { PN531_USB_DRIVER_NAME,  NULL, NULL,    pn531_usb_connect,   pn531_usb_transceive,     pn531_usb_disconnect        },
  { PN533_USB_DRIVER_NAME,  NULL, NULL,    pn533_usb_connect,   pn533_usb_transceive,     pn533_usb_disconnect        },
#endif /* HAVE_LIBUSB */
  { PN532_UART_DRIVER_NAME, NULL, NULL,  pn532_uart_connect,  pn532_uart_transceive,    pn532_uart_disconnect   },
  { ARYGON_DRIVER_NAME,     NULL, NULL,  arygon_connect,      arygon_transceive,        arygon_disconnect       }
};

#endif // __NFC_DRIVERS_H__

