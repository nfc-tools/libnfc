/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#ifndef _LIBNFC_DEVICES_H_
#define _LIBNFC_DEVICES_H_

#include "defines.h"
#include "types.h"
#ifdef HAVE_PCSC_LITE
  #include "dev_acr122.h"
#endif
#include "dev_pn531.h"
#include "dev_pn533.h"
#include "dev_arygon.h"

const static struct dev_callbacks dev_callbacks_list[] = {
//  Driver Name        Connect                  Transceive                    Disconect
#ifdef HAVE_PCSC_LITE
  { "ACR122",          dev_acr122_connect,      dev_acr122_transceive,        dev_acr122_disconnect       },
#endif
  { "PN531USB",        dev_pn531_connect,       dev_pn531_transceive,         dev_pn531_disconnect        },
  { "PN533USB",        dev_pn533_connect,       dev_pn533_transceive,         dev_pn533_disconnect        },
  { "ARYGON",          dev_arygon_connect,      dev_arygon_transceive,        dev_arygon_disconnect       }
};

#endif // _LIBNFC_DEVICES_H_

