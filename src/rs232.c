/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
  Based on rs232-code written by Teunis van Beelen
  available: http://www.teuniz.net/RS-232/index.html
 
*/


#include "rs232.h"

#ifndef _WIN32   /* Linux */

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
  sp->tiNew.c_cc[VMIN] = 0;      // block untill n bytes are received
  sp->tiNew.c_cc[VTIME] = 0;     // block untill a timer expires (n * 100 mSec.)
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

bool rs232_receive(const serial_port sp, byte* pbtRx, uint32_t* puiRxLen)
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
    if (iResult < 0) return false;
  
    // Read time-out
    if (iResult == 0)
    {
      // Test if we at least have received something
      if (uiCount == 0) return false;
      // Store the received byte count and return succesful
      *puiRxLen = uiCount;
      return true;
    }
    
    // There is something available, read the data
    uiCount += read(((serial_port_unix*)sp)->fd,pbtRx+uiCount,*puiRxLen-uiCount);
  }
}

bool rs232_send(const serial_port sp, const byte* pbtTx, const uint32_t uiTxLen)
{
  int iResult;
  iResult = write(((serial_port_unix*)sp)->fd,pbtTx,uiTxLen);
  return (iResult >= 0);
}

#else         /* windows */


HANDLE Cport[16];


char comports[16][10]={"\\\\.\\COM1",  "\\\\.\\COM2",  "\\\\.\\COM3",  "\\\\.\\COM4",
                       "\\\\.\\COM5",  "\\\\.\\COM6",  "\\\\.\\COM7",  "\\\\.\\COM8",
                       "\\\\.\\COM9",  "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                       "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16"};

char baudr[64];


int OpenComport(int comport_number, int baudrate)
{
  if((comport_number>15)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
      sscanf(baudr,"baud=%d data=8 parity=N stop=1",baudrate);
/*
    case     110 : strcpy(baudr, "baud=110 data=8 parity=N stop=1");
                   break;
    case     300 : strcpy(baudr, "baud=300 data=8 parity=N stop=1");
                   break;
    case     600 : strcpy(baudr, "baud=600 data=8 parity=N stop=1");
                   break;
    case    1200 : strcpy(baudr, "baud=1200 data=8 parity=N stop=1");
                   break;
    case    2400 : strcpy(baudr, "baud=2400 data=8 parity=N stop=1");
                   break;
    case    4800 : strcpy(baudr, "baud=4800 data=8 parity=N stop=1");
                   break;
    case    9600 : strcpy(baudr, "baud=9600 data=8 parity=N stop=1");
                   break;
    case   19200 : strcpy(baudr, "baud=19200 data=8 parity=N stop=1");
                   break;
    case   38400 : strcpy(baudr, "baud=38400 data=8 parity=N stop=1");
                   break;
    case   57600 : strcpy(baudr, "baud=57600 data=8 parity=N stop=1");
                   break;
    case  115200 : strcpy(baudr, "baud=115200 data=8 parity=N stop=1");
                   break;
    case  128000 : strcpy(baudr, "baud=128000 data=8 parity=N stop=1");
                   break;
    case  256000 : strcpy(baudr, "baud=256000 data=8 parity=N stop=1");
                   break;
    default      : printf("invalid baudrate\n");
                   return(1);
                   break;
*/
  }

  Cport[comport_number] = CreateFileA(comports[comport_number],
                      GENERIC_READ|GENERIC_WRITE,
                      0,                          /* no share  */
                      NULL,                       /* no security */
                      OPEN_EXISTING,
                      0,                          /* no threads */
                      NULL);                      /* no templates */

  if(Cport[comport_number]==INVALID_HANDLE_VALUE)
  {
    printf("unable to open comport\n");
    return(1);
  }

  DCB port_settings;
  memset(&port_settings, 0, sizeof(port_settings));  /* clear the new struct  */
  port_settings.DCBlength = sizeof(port_settings);

  if(!BuildCommDCBA(baudr, &port_settings))
  {
    printf("unable to set comport dcb settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  if(!SetCommState(Cport[comport_number], &port_settings))
  {
    printf("unable to set comport cfg settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  COMMTIMEOUTS Cptimeouts;

  Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
  Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
  Cptimeouts.ReadTotalTimeoutConstant    = 0;
  Cptimeouts.WriteTotalTimeoutMultiplier = 0;
  Cptimeouts.WriteTotalTimeoutConstant   = 0;

  if(!SetCommTimeouts(Cport[comport_number], &Cptimeouts))
  {
    printf("unable to set comport time-out settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  return(0);
}


int PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(size>4096)  size = 4096;

/* added the void pointer cast, otherwise gcc will complain about */
/* "warning: dereferencing type-punned pointer will break strict aliasing rules" */

  ReadFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL);

  return(n);
}


int SendByte(int comport_number, unsigned char byte)
{
  int n;

  WriteFile(Cport[comport_number], &byte, 1, (LPDWORD)((void *)&n), NULL);

  if(n<0)  return(1);

  return(0);
}


int SendBuf(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(WriteFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL))
  {
    return(n);
  }

  return(-1);
}

bool rs232_cts(const serial_port sp);
{
  int status;
  
  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));
  
  if(status&MS_CTS_ON) return(1);
  else return(0);
}


void CloseComport(int comport_number)
{
  CloseHandle(Cport[comport_number]);
}


#endif
