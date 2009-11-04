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
 * @file devices.h
 * @brief
 */

#ifndef _LIBNFC_DEVICES_H_
#define _LIBNFC_DEVICES_H_

#include "defines.h"
#include "types.h"
#ifdef HAVE_PCSC_LITE
  #include "drivers/acr122.h"
#endif /* HAVE_PCSC_LITE */
#ifdef HAVE_LIBUSB
  #include "drivers/pn531.h"
  #include "drivers/pn533.h"
#endif /* HAVE_LIBUSB */
#include "drivers/arygon.h"
#include "drivers/pn532_uart.h"

const static struct driver_callbacks drivers_callbacks_list[] = {
//  Driver Name        Connect                  Transceive                    Disconnect
#ifdef HAVE_PCSC_LITE
  { "ACR122",          acr122_connect,      acr122_transceive,        acr122_disconnect       },
#endif /* HAVE_PCSC_LITE */
#ifdef HAVE_LIBUSB
  { "PN531USB",        pn531_connect,       pn531_transceive,         pn531_disconnect        },
  { "PN533USB",        pn533_connect,       pn533_transceive,         pn533_disconnect        },
#endif /* HAVE_LIBUSB */
  { "PN532_UART",      pn532_uart_connect,  pn532_uart_transceive,    pn532_uart_disconnect   },
  { "ARYGON",          arygon_connect,      arygon_transceive,        arygon_disconnect       }
};

#endif // _LIBNFC_DEVICES_H_

