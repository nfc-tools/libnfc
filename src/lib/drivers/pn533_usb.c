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
 * @file pn533_usb.c
 * @brief Driver for PN533 chip using USB
 */

/*
Thanks to d18c7db and Okko for example code
*/

#include <sys/param.h>
#include <stdio.h>
#include <usb.h>
#include <string.h>

#include "pn533_usb.h"
#include "../drivers.h"

#include "nfc-messages.h"

#define BUFFER_LENGTH 256
#define USB_TIMEOUT   30000

typedef struct {
  usb_dev_handle* pudh;
  uint32_t uiEndPointIn;
  uint32_t uiEndPointOut;
} usb_spec_t;

// Find transfer endpoints for bulk transfers
static void get_end_points(struct usb_device *dev, usb_spec_t* pus)
{
  uint32_t uiIndex;
  uint32_t uiEndPoint;
  struct usb_interface_descriptor* puid = dev->config->interface->altsetting;

  // 3 Endpoints maximum: Interrupt In, Bulk In, Bulk Out
  for(uiIndex = 0; uiIndex < puid->bNumEndpoints; uiIndex++)
  {
    // Only accept bulk transfer endpoints (ignore interrupt endpoints)
    if(puid->endpoint[uiIndex].bmAttributes != USB_ENDPOINT_TYPE_BULK) continue;

    // Copy the endpoint to a local var, makes it more readable code
    uiEndPoint = puid->endpoint[uiIndex].bEndpointAddress;

    // Test if we dealing with a bulk IN endpoint
    if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN)
    {
      #ifdef DEBUG
        printf("Bulk endpoint in  : 0x%02X\n", uiEndPoint);
      #endif
      pus->uiEndPointIn = uiEndPoint;
    }

    // Test if we dealing with a bulk OUT endpoint
    if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT)
    {
      #ifdef DEBUG
        printf("Bulk endpoint in  : 0x%02X\n", uiEndPoint);
      #endif
      pus->uiEndPointOut = uiEndPoint;
    }
  }
}

nfc_device_t* pn533_usb_connect(const nfc_device_desc_t* pndd)
{
  int idvendor = 0x04e6;
  int idproduct = 0x5591;
  struct usb_bus *bus;
  struct usb_device *dev;
  nfc_device_t* pnd = NULL;
  usb_spec_t* pus;
  usb_spec_t us;
  uint32_t uiDevIndex;

  us.uiEndPointIn = 0;
  us.uiEndPointOut = 0;
  us.pudh = NULL;

  usb_init();
  if (usb_find_busses() < 0) return NULL;
  if (usb_find_devices() < 0) return NULL;

  // Initialize the device index we are seaching for
  if( pndd == NULL ) {
    uiDevIndex = 0;
  } else {
    uiDevIndex = pndd->uiBusIndex;
  }

  for (bus = usb_get_busses(); bus; bus = bus->next)
  {
    for (dev = bus->devices; dev; dev = dev->next)
    {
      if (idvendor==dev->descriptor.idVendor && idproduct==dev->descriptor.idProduct)
      {
        // Make sure there are 2 endpoints available
        if (dev->config->interface->altsetting->bNumEndpoints < 2) return pnd;

        // Test if we are looking for this device according to the current index
        if (uiDevIndex != 0)
        {
          // Nope, we maybe want the next one, let's try to find another
          uiDevIndex--;
          continue;
        }
        DBG("%s", "Found PN533 device");

        // Open the PN533 USB device
        us.pudh = usb_open(dev);

        get_end_points(dev,&us);
        if(usb_set_configuration(us.pudh,1) < 0)
        {
          DBG("%s", "Setting config failed");
          usb_close(us.pudh);
          return NULL;
        }

        if(usb_claim_interface(us.pudh,0) < 0)
        {
          DBG("%s", "Can't claim interface");
          usb_close(us.pudh);
          // don't return yet as there might be other readers on USB bus
          continue;
        }
        // Allocate memory for the device info and specification, fill it and return the info
        pus = malloc(sizeof(usb_spec_t));
        *pus = us;
        pnd = malloc(sizeof(nfc_device_t));
        strcpy(pnd->acName,"PN533USB");
        pnd->nc = NC_PN533;
        pnd->nds = (nfc_device_spec_t)pus;
        pnd->bActive = true;
        pnd->bCrc = true;
        pnd->bPar = true;
        pnd->ui8TxBits = 0;
        return pnd;
      }
    }
  }
  return pnd;
}

void pn533_usb_disconnect(nfc_device_t* pnd)
{
  usb_spec_t* pus = (usb_spec_t*)pnd->nds;
  usb_release_interface(pus->pudh,0);
  usb_close(pus->pudh);
  free(pnd->nds);
  free(pnd);
}

bool pn533_usb_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
{
  size_t uiPos = 0;
  int ret = 0;
  byte_t abtTx[BUFFER_LENGTH] = { 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"
  byte_t abtRx[BUFFER_LENGTH];
  usb_spec_t* pus = (usb_spec_t*)nds;

  // Packet length = data length (len) + checksum (1) + end of stream marker (1)
  abtTx[3] = szTxLen;
  // Packet length checksum 
  abtTx[4] = BUFFER_LENGTH - abtTx[3];
  // Copy the PN53X command into the packet abtTx
  memmove(abtTx+5,pbtTx,szTxLen);

  // Calculate data payload checksum
  abtTx[szTxLen+5] = 0;
  for(uiPos=0; uiPos < szTxLen; uiPos++) 
  {
    abtTx[szTxLen+5] -= abtTx[uiPos+5];
  }

  // End of stream marker
  abtTx[szTxLen+6] = 0;

  #ifdef DEBUG
    printf(" TX: ");
    print_hex(abtTx,szTxLen+7);
  #endif

  ret = usb_bulk_write(pus->pudh, pus->uiEndPointOut, (char*)abtTx, szTxLen+7, USB_TIMEOUT);
  if( ret < 0 )
  {
    #ifdef DEBUG
      printf("usb_bulk_write failed with error %d\n", ret);
    #endif
    return false;
  }

  ret = usb_bulk_read(pus->pudh, pus->uiEndPointIn, (char*)abtRx, BUFFER_LENGTH, USB_TIMEOUT);
  if( ret < 0 )
  {
    #ifdef DEBUG
      printf( "usb_bulk_read failed with error %d\n", ret);
    #endif
    return false;
  }

  #ifdef DEBUG
    printf(" RX: ");
    print_hex(abtRx,ret);
  #endif

  if( ret == 6 )
  {
    ret = usb_bulk_read(pus->pudh, pus->uiEndPointIn, (char*)abtRx, BUFFER_LENGTH, USB_TIMEOUT);
    if( ret < 0 )
    {
      #ifdef DEBUG
        printf("usb_bulk_read failed with error %d\n", ret);
      #endif
      return false;
    }

    #ifdef DEBUG
      printf(" RX: ");
      print_hex(abtRx,ret);
    #endif
  }

  // When the answer should be ignored, just return a succesful result
  if(pbtRx == NULL || pszRxLen == NULL) return true;

  // Only succeed when the result is at least 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
  if(ret < 9) return false;

  // Remove the preceding and appending bytes 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
  *pszRxLen = ret - 7 - 2;

  // Get register: nuke extra byte (awful hack)
  if ((abtRx[5]==0xd5) && (abtRx[6]==0x07) && (*pszRxLen==2)) {
      // printf("Got %02x %02x, keep %02x\n", abtRx[7], abtRx[8], abtRx[8]);
      *pszRxLen = (*pszRxLen) - 1;
      memcpy( pbtRx, abtRx + 8, *pszRxLen);
      return true;
  }

  memcpy( pbtRx, abtRx + 7, *pszRxLen);

  return true;
}
