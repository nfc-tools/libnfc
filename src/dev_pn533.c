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

/*
Thanks to d18c7db and Okko for example code
*/

#include "defines.h"

#include <usb.h>
#include <stdio.h>
#include <string.h>
#include "dev_pn533.h"
#include "bitutils.h"

#define BUFFER_LENGTH 256
#define USB_TIMEOUT   30000
static char buffer[BUFFER_LENGTH] = { 0x00, 0x00, 0xff }; // Every packet must start with "00 00 ff"

typedef struct {
  usb_dev_handle* pudh;
  uint32_t uiEndPointIn;
  uint32_t uiEndPointOut;
} dev_spec_pn533;

// Find transfer endpoints for bulk transfers
static void get_end_points(struct usb_device *dev, dev_spec_pn533* pdsp)
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
      pdsp->uiEndPointIn = uiEndPoint;
    }

    // Test if we dealing with a bulk OUT endpoint
    if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT)
    {
      #ifdef DEBUG
        printf("Bulk endpoint in  : 0x%02X\n", uiEndPoint);
      #endif
      pdsp->uiEndPointOut = uiEndPoint;
    }
  }
}

dev_info* dev_pn533_connect(const nfc_device_desc_t* device_desc)
{
  int idvendor = 0x04e6;
  int idproduct = 0x5591;
  struct usb_bus *bus;
  struct usb_device *dev;
  dev_info* pdi = INVALID_DEVICE_INFO;
  dev_spec_pn533* pdsp;
  dev_spec_pn533 dsp;
  uint32_t uiDevIndex;

  dsp.uiEndPointIn = 0;
  dsp.uiEndPointOut = 0;
  dsp.pudh = NULL;

  usb_init();
  if (usb_find_busses() < 0) return INVALID_DEVICE_INFO;
  if (usb_find_devices() < 0) return INVALID_DEVICE_INFO;

  // Initialize the device index we are seaching for
  if( device_desc == NULL ) {
    uiDevIndex = 0;
  } else {
    uiDevIndex = device_desc->index;
  }

  for (bus = usb_get_busses(); bus; bus = bus->next)
  {
    for (dev = bus->devices; dev; dev = dev->next)
    {
      if (idvendor==dev->descriptor.idVendor && idproduct==dev->descriptor.idProduct)
      {
        // Make sure there are 2 endpoints available
        if (dev->config->interface->altsetting->bNumEndpoints < 2) return pdi;

        // Test if we are looking for this device according to the current index
        if (uiDevIndex != 0)
        {
          // Nope, we maybe want the next one, let's try to find another
          uiDevIndex--;
          continue;
        }
        #ifdef DEBUG
          printf("Found PN533 device\n");
        #endif

        // Open the PN533 USB device
        dsp.pudh = usb_open(dev);

        get_end_points(dev,&dsp);                                                       
        if(usb_set_configuration(dsp.pudh,1) < 0)                                  
        {                                                                          
          #ifdef DEBUG
            printf("Setting config failed\n");                                     
          #endif
          usb_close(dsp.pudh);                                                      
          return INVALID_DEVICE_INFO;
        }

        if(usb_claim_interface(dsp.pudh,0) < 0)
        {
          #ifdef DEBUG
            printf("Can't claim interface\n");
          #endif
          usb_close(dsp.pudh);
          return INVALID_DEVICE_INFO;
        }
        // Allocate memory for the device info and specification, fill it and return the info
        pdsp = malloc(sizeof(dev_spec_pn533));
        *pdsp = dsp;
        pdi = malloc(sizeof(dev_info));
        strcpy(pdi->acName,"PN533USB");
        pdi->ct = CT_PN533;
        pdi->ds = (dev_spec)pdsp;
        pdi->bActive = true;
        pdi->bCrc = true;
        pdi->bPar = true;
        pdi->ui8TxBits = 0;
        return pdi;
      }
    }
  }
  return pdi;
}                                                                                          

void dev_pn533_disconnect(dev_info* pdi)
{
  dev_spec_pn533* pdsp = (dev_spec_pn533*)pdi->ds;
  usb_release_interface(pdsp->pudh,0);
	usb_close(pdsp->pudh);
  free(pdi->ds);
  free(pdi);
}                                        

bool dev_pn533_transceive(const dev_spec ds, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen)
{                                                                          
    uint32_t uiPos = 0;                                                             
    int ret = 0;                                                           
    char buf[BUFFER_LENGTH];
    dev_spec_pn533* pdsp = (dev_spec_pn533*)ds;

    // Packet length = data length (len) + checksum (1) + end of stream marker (1)
    buffer[3] = uiTxLen;                                                                
    // Packet length checksum 
    buffer[4] = BUFFER_LENGTH - buffer[3];                                                  
    // Copy the PN53X command into the packet buffer
    memmove(buffer+5,pbtTx,uiTxLen);

    // Calculate data payload checksum
    buffer[uiTxLen+5] = 0;                   
    for(uiPos=0; uiPos < uiTxLen; uiPos++) 
    {
      buffer[uiTxLen+5] -= buffer[uiPos+5];
    }

    // End of stream marker
    buffer[uiTxLen+6] = 0;        

    #ifdef DEBUG
      printf("Tx: ");
      print_hex((byte_t*)buffer,uiTxLen+7);
    #endif

    ret = usb_bulk_write(pdsp->pudh, pdsp->uiEndPointOut, buffer, uiTxLen+7, USB_TIMEOUT);
    if( ret < 0 )
    {
      #ifdef DEBUG
        printf("usb_bulk_write failed with error %d\n", ret);
      #endif
      return false;
    }

    ret = usb_bulk_read(pdsp->pudh, pdsp->uiEndPointIn, buf, BUFFER_LENGTH, USB_TIMEOUT);
    if( ret < 0 )
    {
      #ifdef DEBUG
        printf( "usb_bulk_read failed with error %d\n", ret);
      #endif
      return false;
    }

    #ifdef DEBUG
      printf("Rx: ");
      print_hex((byte_t*)buf,ret);
    #endif

    if( ret == 6 )
    {
      ret = usb_bulk_read(pdsp->pudh, pdsp->uiEndPointIn, buf, BUFFER_LENGTH, USB_TIMEOUT);
      if( ret < 0 )
      {
        #ifdef DEBUG
          printf("usb_bulk_read failed with error %d\n", ret);
        #endif
        return false;
      }

      #ifdef DEBUG
        printf("Rx: ");
        print_hex((byte_t*)buf,ret);
      #endif
    }

    // When the answer should be ignored, just return a succesful result    
    if(pbtRx == NULL || puiRxLen == NULL) return true;

    // Only succeed when the result is at least 00 00 FF xx Fx Dx xx .. .. .. xx 00 (x = variable)
    if(ret < 9) return false;

    // Remove the preceding and appending bytes 00 00 FF xx Fx .. .. .. xx 00 (x = variable)
    *puiRxLen = ret - 7 - 2;

    // Get register: nuke extra byte (awful hack)
    if ((buf[5]==(char)0xd5) && (buf[6]==(char)0x07) && (*puiRxLen==2)) {
        // printf("Got %02x %02x, keep %02x\n", buf[7], buf[8], buf[8]);
        *puiRxLen = (*puiRxLen) - 1;
        memcpy( pbtRx, buf + 8, *puiRxLen);
        return true;
    }

    memcpy( pbtRx, buf + 7, *puiRxLen);

    return true;
}
