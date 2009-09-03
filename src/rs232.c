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
Based on rs232-code written by Teunis van Beelen
available: http://www.teuniz.net/RS-232/index.html
*/


#include "rs232.h"

// Test if we are dealing with unix operating systems
#ifndef _WIN32

typedef struct termios term_info;
typedef struct { 
  int fd;           // Serial port file descriptor
  term_info tiOld;  // Terminal info before using the port
  term_info tiNew;  // Terminal info during the transaction
} serial_port_unix;

// Set time-out on 30 miliseconds
struct timeval tv = { 
  .tv_sec = 0,      // No seconds
  .tv_usec = 30000 // 30,000 micro seconds
};

// Work-around to claim rs232 interface using the c_iflag (software input processing) from the termios struct
#define CCLAIMED 0x80000000

serial_port rs232_open(const char* pcPortName)
{
  serial_port_unix* sp = malloc(sizeof(serial_port_unix));
  
  if (sp == 0) return INVALID_SERIAL_PORT;
  
  sp->fd = open(pcPortName, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
  if(sp->fd == -1)
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }
  
  if(tcgetattr(sp->fd,&sp->tiOld) == -1)
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }
  
  // Make sure the port is not claimed already
  if (sp->tiOld.c_iflag & CCLAIMED)
  {
    rs232_close(sp);
    return CLAIMED_SERIAL_PORT;
  }
  
  // Copy the old terminal info struct
  sp->tiNew = sp->tiOld;

  sp->tiNew.c_cflag = CS8 | CLOCAL | CREAD;
  sp->tiNew.c_iflag = CCLAIMED | IGNPAR;
  sp->tiNew.c_oflag = 0;
  sp->tiNew.c_lflag = 0;

  /**
   * @note ARYGON-ADRA (PN531): ???,n,8,1
   * @note ARYGON-ADRB (PN532): ???,n,8,1
   * @note ARYGON-APDA (PN531): 9600,n,8,1
   * @note ARYGON-APDB (PN532): 115200,n,8,1
   */
  /** @todo provide this settings dynamically */
#define DEBUG__TRY_ARYGON_APDB

#ifdef DEBUG__TRY_ARYGON_APDB
  cfsetispeed(&(sp->tiNew), B115200);
  cfsetospeed(&(sp->tiNew), B115200);
#endif

  sp->tiNew.c_cc[VMIN] = 0;      // block until n bytes are received
  sp->tiNew.c_cc[VTIME] = 0;     // block until a timer expires (n * 100 mSec.)
  if(tcsetattr(sp->fd,TCSANOW,&sp->tiNew) == -1)
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }
  return sp;
}

void rs232_close(const serial_port sp)
{
  tcsetattr(((serial_port_unix*)sp)->fd,TCSANOW,&((serial_port_unix*)sp)->tiOld);
  close(((serial_port_unix*)sp)->fd);
  free(sp);
}

bool rs232_cts(const serial_port sp)
{
  char status;
  if (ioctl(((serial_port_unix*)sp)->fd,TIOCMGET,&status) < 0) return false;
  return (status & TIOCM_CTS);
}

bool rs232_receive(const serial_port sp, byte_t* pbtRx, uint32_t* puiRxLen)
{
  int iResult;
  uint32_t uiCount = 0;
  fd_set rfds;

  while (true)
  {
    // Reset file descriptor
    FD_ZERO(&rfds);
    FD_SET(((serial_port_unix*)sp)->fd,&rfds);
    iResult = select(((serial_port_unix*)sp)->fd+1, &rfds, NULL, NULL, &tv);

    // Read error
    if (iResult < 0) {
      DBG("RX error.");
      return false;
    }

    // Read time-out
    if (iResult == 0)
    {
      // Test if we at least have received something
      if (uiCount == 0) {
        DBG("RX time-out without received data.");
        return false;
      }

      // Store the received byte count and return succesful
      *puiRxLen = uiCount;
      return true;
    }

    // There is something available, read the data
    uiCount += read(((serial_port_unix*)sp)->fd,pbtRx+uiCount,*puiRxLen-uiCount);
  }
}

bool rs232_send(const serial_port sp, const byte_t* pbtTx, const uint32_t uiTxLen)
{
  int iResult;
  iResult = write(((serial_port_unix*)sp)->fd,pbtTx,uiTxLen);
  return (iResult >= 0);
}

#else
// The windows serial port implementation

typedef struct { 
  HANDLE hPort;     // Serial port handle
  DCB dcb;          // Device control settings
  COMMTIMEOUTS ct;  // Serial port time-out configuration
} serial_port_windows;

serial_port rs232_open(const char* pcPortName)
{
  char acPortName[255];
  serial_port_windows* sp = malloc(sizeof(serial_port_windows));

  // Copy the input "com?" to "\\.\COM?" format
  sprintf(acPortName,"\\\\.\\%s",pcPortName);
  _strupr(acPortName);

  // Try to open the serial port
  sp->hPort = CreateFileA(acPortName,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
  if (sp->hPort == INVALID_HANDLE_VALUE)
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }

  // Prepare the device control
  memset(&sp->dcb, 0, sizeof(DCB));
  sp->dcb.DCBlength = sizeof(DCB);
  if(!BuildCommDCBA("baud=9600 data=8 parity=N stop=1",&sp->dcb))
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }

  // Update the active serial port
  if(!SetCommState(sp->hPort,&sp->dcb))
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }

  sp->ct.ReadIntervalTimeout         = 0;
  sp->ct.ReadTotalTimeoutMultiplier  = 0;
  sp->ct.ReadTotalTimeoutConstant    = 30;
  sp->ct.WriteTotalTimeoutMultiplier = 0;
  sp->ct.WriteTotalTimeoutConstant   = 30;

  if(!SetCommTimeouts(sp->hPort,&sp->ct))
  {
    rs232_close(sp);
    return INVALID_SERIAL_PORT;
  }

  return sp;
}

void rs232_close(const serial_port sp)
{
  CloseHandle(((serial_port_windows*)sp)->hPort);
  free(sp);
}

bool rs232_cts(const serial_port sp)
{
  DWORD dwStatus;
  if (!GetCommModemStatus(((serial_port_windows*)sp)->hPort,&dwStatus)) return false;
  return (dwStatus & MS_CTS_ON);
}

bool rs232_receive(const serial_port sp, byte_t* pbtRx, uint32_t* puiRxLen)
{
  ReadFile(((serial_port_windows*)sp)->hPort,pbtRx,*puiRxLen,(LPDWORD)puiRxLen,NULL);
  return (*puiRxLen != 0);
}

bool rs232_send(const serial_port sp, const byte_t* pbtTx, const uint32_t uiTxLen)
{
  DWORD dwTxLen = 0;
  return WriteFile(((serial_port_windows*)sp)->hPort,pbtTx,uiTxLen,&dwTxLen,NULL);
  return (dwTxLen != 0);
}

#endif
