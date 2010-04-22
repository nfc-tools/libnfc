/*-
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
 */

/**
 * @file drivers.h
 * @brief Supported drivers header
 */

#ifndef __NFC_DRIVERS_H__
#define __NFC_DRIVERS_H__

#include <nfc/nfc-types.h>

#if defined (DRIVER_ACR122_ENABLED)
  #include "drivers/acr122.h"
#endif /* DRIVER_ACR122_ENABLED */

#if defined (DRIVER_PN531_USB_ENABLED) || defined (DRIVER_PN533_USB_ENABLED)
  #include "drivers/pn53x_usb.h"
#endif /* DRIVER_PN531_USB_ENABLED || DRIVER_PN533_USB_ENABLED */

#if defined (DRIVER_PN531_USB_ENABLED)
  #include "drivers/pn531_usb.h"
#endif /* DRIVER_PN531_USB_ENABLED */

#if defined (DRIVER_PN533_USB_ENABLED)
  #include "drivers/pn533_usb.h"
#endif /* DRIVER_PN533_USB_ENABLED */

#if defined (DRIVER_ARYGON_ENABLED)
  #include "drivers/arygon.h"
#endif /* DRIVER_ARYGON_ENABLED */

#if defined (DRIVER_PN532_UART_ENABLED)
  #include "drivers/pn532_uart.h"
#endif /* DRIVER_PN532_UART_ENABLED */

#define DRIVERS_MAX_DEVICES         16
#define MAX_FRAME_LEN       264

static const struct driver_callbacks drivers_callbacks_list[] = {
//  Driver Name             Pick Device             List Devices              Connect              Transceive                Disconnect
#if defined (DRIVER_ACR122_ENABLED)
  { ACR122_DRIVER_NAME,     acr122_pick_device,     acr122_list_devices,      acr122_connect,      acr122_transceive,        acr122_disconnect       },
#endif /* DRIVER_ACR122_ENABLED */
#if defined (DRIVER_PN531_USB_ENABLED)
  { PN531_USB_DRIVER_NAME,  pn531_usb_pick_device, pn531_usb_list_devices,    pn531_usb_connect,   pn53x_usb_transceive,     pn53x_usb_disconnect    },
#endif /* DRIVER_PN531_USB_ENABLED */
#if defined (DRIVER_PN533_USB_ENABLED)
  { PN533_USB_DRIVER_NAME,  pn533_usb_pick_device, pn533_usb_list_devices,    pn533_usb_connect,   pn53x_usb_transceive,     pn53x_usb_disconnect    },
#endif /* DRIVER_PN533_USB_ENABLED */
#if defined (DRIVER_ARYGON_ENABLED)
  { ARYGON_DRIVER_NAME,     arygon_pick_device,     arygon_list_devices,      arygon_connect,      arygon_transceive,        arygon_disconnect       },
#endif /* DRIVER_ARYGON_ENABLED */
#if defined (DRIVER_PN532_UART_ENABLED)
  { PN532_UART_DRIVER_NAME, pn532_uart_pick_device, pn532_uart_list_devices,  pn532_uart_connect,  pn532_uart_transceive,    pn532_uart_disconnect   },
#endif /* DRIVER_PN532_UART_ENABLED */
};

#endif // __NFC_DRIVERS_H__

