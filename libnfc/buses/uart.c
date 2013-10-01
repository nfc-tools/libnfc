/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
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
 * @file uart.c
 * @brief UART driver
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "uart.h"

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#include <nfc/nfc.h>
#include "nfc-internal.h"

#define LOG_GROUP    NFC_LOG_GROUP_COM
#define LOG_CATEGORY "libnfc.bus.uart"

#ifndef _WIN32
// Needed by sleep() under Unix
#  include <unistd.h>
#  include <time.h>
#  define msleep(x) do { \
    struct timespec xsleep; \
    xsleep.tv_sec = x / 1000; \
    xsleep.tv_nsec = (x - xsleep.tv_sec * 1000) * 1000 * 1000; \
    nanosleep(&xsleep, NULL); \
  } while (0)
#else
// Needed by Sleep() under Windows
#  include <winbase.h>
#  define msleep Sleep
#endif

#  if defined(__APPLE__)
const char *serial_ports_device_radix[] = { "tty.SLAB_USBtoUART", "tty.usbserial-", NULL };
#  elif defined (__FreeBSD__) || defined (__OpenBSD__) || defined(__FreeBSD_kernel__)
const char *serial_ports_device_radix[] = { "cuaU", "cuau", NULL };
#  elif defined (__linux__)
const char *serial_ports_device_radix[] = { "ttyUSB", "ttyS", "ttyACM", "ttyAMA", "ttyO", NULL };
#  else
#    error "Can't determine serial string for your system"
#  endif

// Work-around to claim uart interface using the c_iflag (software input processing) from the termios struct
#  define CCLAIMED 0x80000000

struct serial_port_unix {
  int 			fd; 			// Serial port file descriptor
  struct termios 	termios_backup; 	// Terminal info before using the port
  struct termios 	termios_new; 		// Terminal info during the transaction
};

#define UART_DATA( X ) ((struct serial_port_unix *) X)

void uart_close_ext(const serial_port sp, const bool restore_termios);

serial_port
uart_open(const char *pcPortName)
{
  struct serial_port_unix *sp = malloc(sizeof(struct serial_port_unix));

  if (sp == 0)
    return INVALID_SERIAL_PORT;

  sp->fd = open(pcPortName, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (sp->fd == -1) {
    uart_close_ext(sp, false);
    return INVALID_SERIAL_PORT;
  }

  if (tcgetattr(sp->fd, &sp->termios_backup) == -1) {
    uart_close_ext(sp, false);
    return INVALID_SERIAL_PORT;
  }
  // Make sure the port is not claimed already
  if (sp->termios_backup.c_iflag & CCLAIMED) {
    uart_close_ext(sp, false);
    return CLAIMED_SERIAL_PORT;
  }
  // Copy the old terminal info struct
  sp->termios_new = sp->termios_backup;

  sp->termios_new.c_cflag = CS8 | CLOCAL | CREAD;
  sp->termios_new.c_iflag = CCLAIMED | IGNPAR;
  sp->termios_new.c_oflag = 0;
  sp->termios_new.c_lflag = 0;

  sp->termios_new.c_cc[VMIN] = 0;     // block until n bytes are received
  sp->termios_new.c_cc[VTIME] = 0;    // block until a timer expires (n * 100 mSec.)

  if (tcsetattr(sp->fd, TCSANOW, &sp->termios_new) == -1) {
    uart_close_ext(sp, true);
    return INVALID_SERIAL_PORT;
  }
  return sp;
}

void
uart_flush_input(serial_port sp, bool wait)
{
  // flush commands may seem to be without effect
  // if asked too quickly after previous event, cf comments below
  // therefore a "wait" argument allows now to wait before flushing
  // I believe that now the byte-eater part is not required anymore --Phil
  if (wait) {
    msleep(50); // 50 ms
  }

  // This line seems to produce absolutely no effect on my system (GNU/Linux 2.6.35)
  tcflush(UART_DATA(sp)->fd, TCIFLUSH);
  // So, I wrote this byte-eater
  // Retrieve the count of the incoming bytes
  int available_bytes_count = 0;
  int res;
  res = ioctl(UART_DATA(sp)->fd, FIONREAD, &available_bytes_count);
  if (res != 0) {
    return;
  }
  if (available_bytes_count == 0) {
    return;
  }
  char *rx = malloc(available_bytes_count);
  if (!rx) {
    perror("malloc");
    return;
  }
  // There is something available, read the data
  if (read(UART_DATA(sp)->fd, rx, available_bytes_count) < 0) {
    perror("uart read");
    free(rx);
    return;
  }
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%d bytes have eaten.", available_bytes_count);
  free(rx);
}

void
uart_set_speed(serial_port sp, const uint32_t uiPortSpeed)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Serial port speed requested to be set to %d bauds.", uiPortSpeed);

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
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to set serial port speed to %d bauds. Speed value must be one of those defined in termios(3).",
              uiPortSpeed);
      return;
  };

  // Set port speed (Input and Output)
  cfsetispeed(&(UART_DATA(sp)->termios_new), stPortSpeed);
  cfsetospeed(&(UART_DATA(sp)->termios_new), stPortSpeed);
  if (tcsetattr(UART_DATA(sp)->fd, TCSADRAIN, &(UART_DATA(sp)->termios_new)) == -1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to apply new speed settings.");
  }
}

uint32_t
uart_get_speed(serial_port sp)
{
  uint32_t uiPortSpeed = 0;
  switch (cfgetispeed(&UART_DATA(sp)->termios_new)) {
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
uart_close_ext(const serial_port sp, const bool restore_termios)
{
  if (UART_DATA(sp)->fd >= 0) {
    if (restore_termios)
      tcsetattr(UART_DATA(sp)->fd, TCSANOW, &UART_DATA(sp)->termios_backup);
    close(UART_DATA(sp)->fd);
  }
  free(sp);
}

void
uart_close(const serial_port sp)
{
  uart_close_ext(sp, true);
}

/**
 * @brief Receive data from UART and copy data to \a pbtRx
 *
 * @return 0 on success, otherwise driver error code
 */
int
uart_receive(serial_port sp, uint8_t *pbtRx, const size_t szRx, void *abort_p, int timeout)
{
  int iAbortFd = abort_p ? *((int *)abort_p) : 0;
  int received_bytes_count = 0;
  int available_bytes_count = 0;
  const int expected_bytes_count = (int)szRx;
  int res;
  fd_set rfds;
  do {
select:
    // Reset file descriptor
    FD_ZERO(&rfds);
    FD_SET(UART_DATA(sp)->fd, &rfds);

    if (iAbortFd) {
      FD_SET(iAbortFd, &rfds);
    }

    struct timeval timeout_tv;
    if (timeout > 0) {
      timeout_tv.tv_sec = (timeout / 1000);
      timeout_tv.tv_usec = ((timeout % 1000) * 1000);
    }

    res = select(MAX(UART_DATA(sp)->fd, iAbortFd) + 1, &rfds, NULL, NULL, timeout ? &timeout_tv : NULL);

    if ((res < 0) && (EINTR == errno)) {
      // The system call was interupted by a signal and a signal handler was
      // run.  Restart the interupted system call.
      goto select;
    }

    // Read error
    if (res < 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Error: %s", strerror(errno));
      return NFC_EIO;
    }
    // Read time-out
    if (res == 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Timeout!");
      return NFC_ETIMEOUT;
    }

    if (FD_ISSET(iAbortFd, &rfds)) {
      // Abort requested
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Abort!");
      close(iAbortFd);
      return NFC_EOPABORTED;
    }

    // Retrieve the count of the incoming bytes
    res = ioctl(UART_DATA(sp)->fd, FIONREAD, &available_bytes_count);
    if (res != 0) {
      return NFC_EIO;
    }
    // There is something available, read the data
    res = read(UART_DATA(sp)->fd, pbtRx + received_bytes_count, MIN(available_bytes_count, (expected_bytes_count - received_bytes_count)));
    // Stop if the OS has some troubles reading the data
    if (res <= 0) {
      return NFC_EIO;
    }
    received_bytes_count += res;

  } while (expected_bytes_count > received_bytes_count);
  LOG_HEX(LOG_GROUP, "RX", pbtRx, szRx);
  return NFC_SUCCESS;
}

/**
 * @brief Send \a pbtTx content to UART
 *
 * @return 0 on success, otherwise a driver error is returned
 */
int
uart_send(serial_port sp, const uint8_t *pbtTx, const size_t szTx, int timeout)
{
  (void) timeout;
  LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);
  if ((int) szTx == write(UART_DATA(sp)->fd, pbtTx, szTx))
    return NFC_SUCCESS;
  else
    return NFC_EIO;
}

char **
uart_list_ports(void)
{
  char **res = malloc(sizeof(char *));
  if (!res) {
    perror("malloc");
    return res;
  }
  size_t szRes = 1;

  res[0] = NULL;
  DIR *dir;
  if ((dir = opendir("/dev")) == NULL) {
    perror("opendir error: /dev");
    return res;
  }
  struct dirent entry;
  struct dirent *result;
  while ((readdir_r(dir, &entry, &result) == 0) && (result != NULL)) {
#if !defined(__APPLE__)
    if (!isdigit(entry.d_name[strlen(entry.d_name) - 1]))
      continue;
#endif
    const char **p = serial_ports_device_radix;
    while (*p) {
      if (!strncmp(entry.d_name, *p, strlen(*p))) {
        char **res2 = realloc(res, (szRes + 1) * sizeof(char *));
        if (!res2) {
          perror("malloc");
          goto oom;
        }
        res = res2;
        if (!(res[szRes - 1] = malloc(6 + strlen(entry.d_name)))) {
          perror("malloc");
          goto oom;
        }
        sprintf(res[szRes - 1], "/dev/%s", entry.d_name);

        szRes++;
        res[szRes - 1] = NULL;
      }
      p++;
    }
  }
oom:
  closedir(dir);

  return res;
}
