/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, 2010, Roel Verdult, Romuald Conty
 * Copyright (C) 2010, Roel Verdult, Romuald Conty
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
 *
 */

/**
 * @file uart_posix.c
 * @brief POSIX UART driver
 */

/* vim: set ts=2 sw=2 et: */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "nfc-internal.h"

#  if defined(__APPLE__)
  // FIXME: find UART connection string for PN53X device on Mac OS X when multiples devices are used
char *serial_ports_device_radix[] = { "tty.SLAB_USBtoUART", NULL };
#  elif defined (__FreeBSD__) || defined (__OpenBSD__)
char *serial_ports_device_radix[] = { "cuaU", "cuau", NULL };
#  elif defined (__linux__)
char *serial_ports_device_radix[] = { "ttyUSB", "ttyS", NULL };
#  else
#    error "Can't determine serial string for your system"
#  endif

typedef struct termios term_info;
typedef struct {
  int     fd;                   // Serial port file descriptor
  term_info tiOld;              // Terminal info before using the port
  term_info tiNew;              // Terminal info during the transaction
} serial_port_unix;

// Work-around to claim uart interface using the c_iflag (software input processing) from the termios struct
#  define CCLAIMED 0x80000000

serial_port
uart_open (const char *pcPortName)
{
  serial_port_unix *sp = malloc (sizeof (serial_port_unix));

  if (sp == 0)
    return INVALID_SERIAL_PORT;

  sp->fd = open (pcPortName, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (sp->fd == -1) {
    uart_close (sp);
    return INVALID_SERIAL_PORT;
  }

  if (tcgetattr (sp->fd, &sp->tiOld) == -1) {
    uart_close (sp);
    return INVALID_SERIAL_PORT;
  }
  // Make sure the port is not claimed already
  if (sp->tiOld.c_iflag & CCLAIMED) {
    uart_close (sp);
    return CLAIMED_SERIAL_PORT;
  }
  // Copy the old terminal info struct
  sp->tiNew = sp->tiOld;

  sp->tiNew.c_cflag = CS8 | CLOCAL | CREAD;
  sp->tiNew.c_iflag = CCLAIMED | IGNPAR;
  sp->tiNew.c_oflag = 0;
  sp->tiNew.c_lflag = 0;

  sp->tiNew.c_cc[VMIN] = 0;     // block until n bytes are received
  sp->tiNew.c_cc[VTIME] = 0;    // block until a timer expires (n * 100 mSec.)

  if (tcsetattr (sp->fd, TCSANOW, &sp->tiNew) == -1) {
    uart_close (sp);
    return INVALID_SERIAL_PORT;
  }

  tcflush (sp->fd, TCIFLUSH);
  return sp;
}

void
uart_set_speed (serial_port sp, const uint32_t uiPortSpeed)
{
  DBG ("Serial port speed requested to be set to %d bauds.", uiPortSpeed);
  const serial_port_unix *spu = (serial_port_unix *) sp;

  // Portability note: on some systems, B9600 != 9600 so we have to do
  // uint32_t <=> speed_t associations by hand.
  speed_t stPortSpeed = B9600;
  switch (uiPortSpeed) {
  case 9600:
    stPortSpeed = B9600;
    break;
  case 19200:
    stPortSpeed = B19200;
    break;
  case 38400:
    stPortSpeed = B38400;
    break;
#  ifdef B57600
  case 57600:
    stPortSpeed = B57600;
    break;
#  endif
#  ifdef B115200
  case 115200:
    stPortSpeed = B115200;
    break;
#  endif
#  ifdef B230400
  case 230400:
    stPortSpeed = B230400;
    break;
#  endif
#  ifdef B460800
  case 460800:
    stPortSpeed = B460800;
    break;
#  endif
  default:
    ERR ("Unable to set serial port speed to %d bauds. Speed value must be one of those defined in termios(3).",
         uiPortSpeed);
    return;
  };

  // Set port speed (Input and Output)
  cfsetispeed ((struct termios *) &(spu->tiNew), stPortSpeed);
  cfsetospeed ((struct termios *) &(spu->tiNew), stPortSpeed);
  if (tcsetattr (spu->fd, TCSADRAIN, &(spu->tiNew)) == -1) {
    ERR ("%s", "Unable to apply new speed settings.");
  }
}

uint32_t
uart_get_speed (serial_port sp)
{
  uint32_t uiPortSpeed = 0;
  const serial_port_unix *spu = (serial_port_unix *) sp;
  switch (cfgetispeed (&spu->tiNew)) {
  case B9600:
    uiPortSpeed = 9600;
    break;
  case B19200:
    uiPortSpeed = 19200;
    break;
  case B38400:
    uiPortSpeed = 38400;
    break;
#  ifdef B57600
  case B57600:
    uiPortSpeed = 57600;
    break;
#  endif
#  ifdef B115200
  case B115200:
    uiPortSpeed = 115200;
    break;
#  endif
#  ifdef B230400
  case B230400:
    uiPortSpeed = 230400;
    break;
#  endif
#  ifdef B460800
  case B460800:
    uiPortSpeed = 460800;
    break;
#  endif
  }

  return uiPortSpeed;
}

void
uart_close (const serial_port sp)
{
  if (((serial_port_unix *) sp)->fd >= 0) {
    tcsetattr (((serial_port_unix *) sp)->fd, TCSANOW, &((serial_port_unix *) sp)->tiOld);
    close (((serial_port_unix *) sp)->fd);
  }
  free (sp);
}

static const struct timeval tvTimeout = {
  .tv_sec = 1,
  .tv_usec = 0
};

/**
 * @brief Receive data from UART and copy data to \a pbtRx
 *
 * @return 0 on success, otherwise driver error code
 */
int
uart_receive (serial_port sp, byte_t * pbtRx, const size_t szRx, int iAbortFd)
{
  struct timeval tv = tvTimeout;
  struct timeval *ptv = &tv;
  int received_bytes_count = 0;
  int available_bytes_count = 0;
  const int expected_bytes_count = (int)szRx;
  int res;
  fd_set rfds;
  do {
select:
    // Reset file descriptor
    FD_ZERO (&rfds);
    FD_SET (((serial_port_unix *) sp)->fd, &rfds);

    if (iAbortFd) {
      FD_SET (iAbortFd, &rfds);
      ptv = NULL;
    }

    res = select (MAX(((serial_port_unix *) sp)->fd, iAbortFd) + 1, &rfds, NULL, NULL, ptv);

    if ((res < 0) && (EINTR == errno)) {
      // The system call was interupted by a signal and a signal handler was
      // run.  Restart the interupted system call.
      goto select;
    }

    // Read error
    if (res < 0) {
      DBG ("%s", "RX error.");
      return DEIO;
    }
    // Read time-out
    if (res == 0) {
      DBG ("Timeout!");
      return DETIMEOUT;
    }

    if (FD_ISSET (iAbortFd, &rfds)) {
      // Abort requested
      DBG ("Abort!");
      close (iAbortFd);
      return DEABORT;
    }

    // Retrieve the count of the incoming bytes
    res = ioctl (((serial_port_unix *) sp)->fd, FIONREAD, &available_bytes_count);
    if (res != 0) {
      return DEIO;
    }
    // There is something available, read the data
    // DBG ("expected bytes: %zu, received bytes: %d, available bytes: %d", szRx, received_bytes_count, available_bytes_count);
    res = read (((serial_port_unix *) sp)->fd, pbtRx + received_bytes_count, MIN(available_bytes_count, (expected_bytes_count - received_bytes_count)));
    // Stop if the OS has some troubles reading the data
    if (res <= 0) {
      return DEIO;
    }
    received_bytes_count += res;

  } while (expected_bytes_count > received_bytes_count);
  PRINT_HEX ("RX", pbtRx, szRx);
  return 0;
}

/**
 * @brief Send \a pbtTx content to UART
 *
 * @return 0 on success, otherwise a driver error is returned
 */
int
uart_send (serial_port sp, const byte_t * pbtTx, const size_t szTx)
{
  PRINT_HEX ("TX", pbtTx, szTx);
  if ((int) szTx == write (((serial_port_unix *) sp)->fd, pbtTx, szTx))
    return 0;
  else
    return DEIO;
}

char **
uart_list_ports (void)
{
    char **res = malloc (sizeof (char *));
    size_t szRes = 1;

    res[0] = NULL;

    DIR *pdDir = opendir("/dev");
    struct dirent *pdDirEnt;
    while ((pdDirEnt = readdir(pdDir)) != NULL) {
	if (!isdigit (pdDirEnt->d_name[strlen (pdDirEnt->d_name) - 1]))
	    continue;

	char **p = serial_ports_device_radix;
	while (*p) {
	    if (!strncmp(pdDirEnt->d_name, *p, strlen (*p))) {
		char **res2 = realloc (res, (szRes+1) * sizeof (char *));
		if (!res2)
		    goto oom;
		
		res = res2;
		if (!(res[szRes-1] = malloc (6 + strlen (pdDirEnt->d_name))))
		    goto oom;

		sprintf (res[szRes-1], "/dev/%s", pdDirEnt->d_name);
		
		szRes++;
		res[szRes-1] = NULL;
	    }
	    p++;
	}
    }
oom:
    closedir (pdDir);

    return res;
}
