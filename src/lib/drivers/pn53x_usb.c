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
 * @file pn53x_usb.c
 * @brief Driver common routines for PN53x chips using USB
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif // HAVE_CONFIG_H

/*
Thanks to d18c7db and Okko for example code
*/

#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <string.h>

#include "../drivers.h"
#include "../bitutils.h"

#include <nfc/nfc-messages.h>

#define BUFFER_LENGTH 256
#define USB_TIMEOUT   30000

// Find transfer endpoints for bulk transfers
void get_end_points(struct usb_device *dev, usb_spec_t* pus)
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
      DBG("Bulk endpoint in  : 0x%02X", uiEndPoint);
      pus->uiEndPointIn = uiEndPoint;
    }

    // Test if we dealing with a bulk OUT endpoint
    if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT)
    {
      DBG("Bulk endpoint in  : 0x%02X", uiEndPoint);
      pus->uiEndPointOut = uiEndPoint;
    }
  }
}

bool pn53x_usb_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound,usb_candidate_t candidates[], int num_candidates, char * target_name)
{
  int ret, i;
  
  struct usb_bus *bus;
  struct usb_device *dev;
  usb_dev_handle *udev;
  uint32_t uiBusIndex = 0;
  char string[256];

  string[0]= '\0';
  usb_init();

  if ((ret= usb_find_busses() < 0)) return NULL;
  DBG("%d busses",ret);
  if ((ret= usb_find_devices() < 0)) return NULL;
  DBG("%d devices",ret);

  *pszDeviceFound= 0;

  for (bus = usb_get_busses(); bus; bus = bus->next)
  {
    for (dev = bus->devices; dev; dev = dev->next, uiBusIndex++)
    {
      for(i = 0; i < num_candidates; ++i)
      {
        DBG("Checking device %04x:%04x (%04x:%04x)",dev->descriptor.idVendor,dev->descriptor.idProduct,candidates[i].idVendor,candidates[i].idProduct);
        if (candidates[i].idVendor==dev->descriptor.idVendor && candidates[i].idProduct==dev->descriptor.idProduct)
        {
          // Make sure there are 2 endpoints available
          // with libusb-win32 we got some null pointers so be robust before looking at endpoints:
          if (dev->config == NULL || dev->config->interface == NULL || dev->config->interface->altsetting == NULL)
          {
            // Nope, we maybe want the next one, let's try to find another
            continue;
          }
          if (dev->config->interface->altsetting->bNumEndpoints < 2)
          {
            // Nope, we maybe want the next one, let's try to find another
            continue;
          }
          if (dev->descriptor.iManufacturer || dev->descriptor.iProduct)
          {
            udev = usb_open(dev);
            if(udev)
            {
              usb_get_string_simple(udev, dev->descriptor.iManufacturer, string, sizeof(string));
              if(strlen(string) > 0)
                strcpy(string + strlen(string)," / ");
              usb_get_string_simple(udev, dev->descriptor.iProduct, string + strlen(string), sizeof(string) - strlen(string));
            }
            usb_close(udev);
          }
          if(strlen(string) == 0)
            strcpy(pnddDevices[*pszDeviceFound].acDevice, target_name);
          else
            strcpy(pnddDevices[*pszDeviceFound].acDevice, string);
          pnddDevices[*pszDeviceFound].pcDriver = target_name;
          pnddDevices[*pszDeviceFound].uiBusIndex = uiBusIndex;
          (*pszDeviceFound)++;
          DBG("%s","Match!");
          // Test if we reach the maximum "wanted" devices
          if((*pszDeviceFound) == szDevices) 
          {
            DBG("Found %d devices",*pszDeviceFound);
            return true;
          }
        }
      }
    }
  }
  DBG("Found %d devices",*pszDeviceFound);
  if(*pszDeviceFound)
    return true;
  return false;
}

nfc_device_t* pn53x_usb_connect(const nfc_device_desc_t* pndd,const char * target_name, int target_chip)
{
  nfc_device_t* pnd = NULL;
  usb_spec_t* pus;
  usb_spec_t us;
  struct usb_bus *bus;
  struct usb_device *dev;

  us.uiEndPointIn = 0;
  us.uiEndPointOut = 0;
  us.pudh = NULL;

  uint32_t uiBusIndex;

  // must specify device to connect to
  if(pndd == NULL) return NULL;

  DBG("Connecting %s device",target_name);
  usb_init();

  uiBusIndex= pndd->uiBusIndex;

  DBG("Skipping to device no. %d",uiBusIndex);
  for (bus = usb_get_busses(); bus; bus = bus->next)
  {
    for (dev = bus->devices; dev; dev = dev->next, uiBusIndex--)
    {
      DBG("Checking device %04x:%04x",dev->descriptor.idVendor,dev->descriptor.idProduct);
      if(uiBusIndex == 0)
      {
        DBG("Found device index %d", pndd->uiBusIndex);

        // Open the USB device
        us.pudh = usb_open(dev);

        get_end_points(dev,&us);
        if(usb_set_configuration(us.pudh,1) < 0)
        {
          DBG("%s", "Setting config failed");
          usb_close(us.pudh);
          // we failed to use the specified device
          return NULL;
        }

        if(usb_claim_interface(us.pudh,0) < 0)
        {
          DBG("%s", "Can't claim interface");
          usb_close(us.pudh);
          // we failed to use the specified device
          return NULL;
        }
        // Allocate memory for the device info and specification, fill it and return the info
        pus = malloc(sizeof(usb_spec_t));
        *pus = us;
        pnd = malloc(sizeof(nfc_device_t));
        strcpy(pnd->acName,target_name);
        pnd->nc = target_chip;
        pnd->nds = (nfc_device_spec_t)pus;
        pnd->bActive = true;
        pnd->bCrc = true;
        pnd->bPar = true;
        pnd->ui8TxBits = 0;
        return pnd;
      }
    }
  }
  // We ran out of devices before the index required
  DBG("%s","Device index not found!");
  return NULL;
}

void pn53x_usb_disconnect(nfc_device_t* pnd)
{
  usb_spec_t* pus = (usb_spec_t*)pnd->nds;
  int ret;

  DBG("%s","resetting USB");
  usb_reset(pus->pudh);
  if((ret= usb_release_interface(pus->pudh,0)) < 0)
    DBG("usb_release failed %i",ret);
  if((ret= usb_close(pus->pudh)) < 0)
    DBG("usb_close failed %i",ret);
  free(pnd->nds);
  free(pnd);
  DBG("%s","done!");
}

bool pn53x_usb_transceive(const nfc_device_spec_t nds, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
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

  DBG("%s","pn53x_usb_transceive");
  #ifdef DEBUG
    printf(" TX: ");
    print_hex(abtTx,szTxLen+7);
  #endif

  ret = usb_bulk_write(pus->pudh, pus->uiEndPointOut, (char*)abtTx, szTxLen+7, USB_TIMEOUT);
  if( ret < 0 )
  {
    DBG("usb_bulk_write failed with error %d", ret);
    return false;
  }

  ret = usb_bulk_read(pus->pudh, pus->uiEndPointIn, (char*)abtRx, BUFFER_LENGTH, USB_TIMEOUT);
  if( ret < 0 )
  {
    DBG( "usb_bulk_read failed with error %d", ret);
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
      DBG("usb_bulk_read failed with error %d", ret);
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
  if(ret < 9) 
  {
    DBG("%s","No data");
    return false;
  }

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
