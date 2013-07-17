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
 * Copyright (C) 2013      Evgeny Boger
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
 * @file spi.c
 * @brief SPI driver
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "spi.h"

#include <sys/ioctl.h>
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

#include <nfc/nfc.h>
#include "nfc-internal.h"

#define LOG_GROUP    NFC_LOG_GROUP_COM
#define LOG_CATEGORY "libnfc.bus.spi"

#  if defined(__APPLE__)
const char *spi_ports_device_radix[] = { "spidev", NULL };
#  elif defined (__FreeBSD__) || defined (__OpenBSD__)
const char *spi_ports_device_radix[] = { "spidev", NULL };
#  elif defined (__linux__)
const char *spi_ports_device_radix[] = { "spidev", NULL };
#  else
#    error "Can't determine spi port string for your system"
#  endif


struct spi_port_unix {
  int 			fd; 			// Serial port file descriptor
  //~ struct termios 	termios_backup; 	// Terminal info before using the port
  //~ struct termios 	termios_new; 		// Terminal info during the transaction
};

#define SPI_DATA( X ) ((struct spi_port_unix *) X)


spi_port
spi_open(const char *pcPortName)
{
  struct spi_port_unix *sp = malloc(sizeof(struct spi_port_unix));

  if (sp == 0)
    return INVALID_SPI_PORT;

  sp->fd = open(pcPortName, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (sp->fd == -1) {
    spi_close(sp);
    return INVALID_SPI_PORT;
  }


  return sp;
}



void
spi_set_speed(spi_port sp, const uint32_t uiPortSpeed)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "SPI port speed requested to be set to %d Hz.", uiPortSpeed);
  int ret;
  ret = ioctl(SPI_DATA(sp)->fd, SPI_IOC_WR_MAX_SPEED_HZ, &uiPortSpeed);
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "ret %d", ret);

  if (ret == -1)  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Error setting SPI speed.");

}

void
spi_set_mode(spi_port sp, const uint32_t uiPortMode)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "SPI port mode requested to be set to %d.", uiPortMode);
  int ret;
  ret = ioctl(SPI_DATA(sp)->fd, SPI_IOC_WR_MODE, &uiPortMode);

  if (ret == -1)  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Error setting SPI mode.");

}

uint32_t
spi_get_speed(spi_port sp)
{
  uint32_t uiPortSpeed = 0;

  int ret;
  ret = ioctl(SPI_DATA(sp)->fd, SPI_IOC_RD_MAX_SPEED_HZ, &uiPortSpeed);

  if (ret == -1) log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Error reading SPI speed.");

  return uiPortSpeed;
}


void
spi_close(const spi_port sp)
{
  close(SPI_DATA(sp)->fd);
  free(sp);
}


/**
 * @brief Perform bit reversal on one byte \a x
 *
 * @return reversed byte
 */

static uint8_t
bit_reversal(const uint8_t x)
{
  uint8_t ret = x;
  ret = (((ret & 0xaa) >> 1) | ((ret & 0x55) << 1));
  ret = (((ret & 0xcc) >> 2) | ((ret & 0x33) << 2));
  ret = (((ret & 0xf0) >> 4) | ((ret & 0x0f) << 4));
  return ret;
}





/**
 * @brief Send \a pbtTx content to SPI then receive data from SPI and copy data to \a pbtRx. CS line stays active	 between transfers as well as during transfers.
 *
 * @return 0 on success, otherwise a driver error is returned
 */
int
spi_send_receive(spi_port sp, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, bool lsb_first)
{
  size_t transfers = 0;
  struct spi_ioc_transfer tr[2];


  uint8_t *pbtTxLSB = 0;

  if (szTx) {
    LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);
    if (lsb_first) {
      pbtTxLSB = malloc(szTx * sizeof(uint8_t));
      if (!pbtTxLSB) {
        return NFC_ESOFT;
      }

      size_t i;
      for (i = 0; i < szTx; ++i) {
        pbtTxLSB[i] = bit_reversal(pbtTx[i]);
      }

      pbtTx = pbtTxLSB;
    }

    struct spi_ioc_transfer tr_send = {
      .tx_buf = (unsigned long) pbtTx,
      .rx_buf = 0,
      .len = szTx ,
      .delay_usecs = 0,
      .speed_hz = 0,
      .bits_per_word = 0,
    };
    tr[transfers] = tr_send;

    ++transfers;
  }

  if (szRx) {
    struct spi_ioc_transfer tr_receive = {
      .tx_buf = 0,
      .rx_buf = (unsigned long) pbtRx,
      .len = szRx,
      .delay_usecs = 0,
      .speed_hz = 0,
      .bits_per_word = 0,
    };
    tr[transfers] = tr_receive;
    ++transfers;
  }



  if (transfers) {
    int ret = ioctl(SPI_DATA(sp)->fd, SPI_IOC_MESSAGE(transfers), tr);
    if (szTx && lsb_first) {
      free(pbtTxLSB);
    }

    if (ret != (int)(szRx + szTx)) {
      return NFC_EIO;
    }

    // Reverse received bytes if needed
    if (szRx) {
      if (lsb_first) {
        size_t i;
        for (i = 0; i < szRx; ++i) {
          pbtRx[i] = bit_reversal(pbtRx[i]);
        }
      }

      LOG_HEX(LOG_GROUP, "RX", pbtRx, szRx);
    }
  }


  return NFC_SUCCESS;
}


/**
 * @brief Receive data from SPI and copy data to \a pbtRx
 *
 * @return 0 on success, otherwise driver error code
 */
int
spi_receive(spi_port sp, uint8_t *pbtRx, const size_t szRx, bool lsb_first)
{
  return spi_send_receive(sp, 0, 0, pbtRx, szRx, lsb_first);
}


/**
 * @brief Send \a pbtTx content to SPI
 *
 * @return 0 on success, otherwise a driver error is returned
 */
int
spi_send(spi_port sp, const uint8_t *pbtTx, const size_t szTx, bool lsb_first)
{
  return spi_send_receive(sp, pbtTx, szTx, 0, 0, lsb_first);
}


char **
spi_list_ports(void)
{
  char **res = malloc(sizeof(char *));
  size_t szRes = 1;

  res[0] = NULL;

  DIR *pdDir = opendir("/dev");
  struct dirent *pdDirEnt;
  struct dirent entry;
  struct dirent *result;
  while ((readdir_r(pdDir, &entry, &result) == 0) && (result != NULL)) {
    pdDirEnt = &entry;
#if !defined(__APPLE__)
    if (!isdigit(pdDirEnt->d_name[strlen(pdDirEnt->d_name) - 1]))
      continue;
#endif
    const char **p = spi_ports_device_radix;
    while (*p) {
      if (!strncmp(pdDirEnt->d_name, *p, strlen(*p))) {
        char **res2 = realloc(res, (szRes + 1) * sizeof(char *));
        if (!res2)
          goto oom;

        res = res2;
        if (!(res[szRes - 1] = malloc(6 + strlen(pdDirEnt->d_name))))
          goto oom;

        sprintf(res[szRes - 1], "/dev/%s", pdDirEnt->d_name);

        szRes++;
        res[szRes - 1] = NULL;
      }
      p++;
    }
  }
oom:
  closedir(pdDir);

  return res;
}
