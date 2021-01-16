/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tarti?re
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2013      Laurent Latil
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
 * @file i2c.c
 * @brief I2C driver (implemented / tested for Linux only currently)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H
#include "i2c.h"

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
#ifdef __APPLE__
#include <sys/termios.h>
#else
#include <termios.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <nfc/nfc.h>
#include "nfc-internal.h"

#define LOG_GROUP    NFC_LOG_GROUP_COM
#define LOG_CATEGORY "libnfc.bus.i2c"

#  if defined (__linux__)
const char *i2c_ports_device_radix[] =
{ "i2c-", NULL };
#  else
#    error "Can't determine I2C devices standard names for your system"
#  endif


struct i2c_device_unix {
  int fd;             // I2C device file descriptor
};

#define I2C_DATA( X ) ((struct i2c_device_unix *) X)

/**
 * @brief Open an I2C device
 *
 * @param pcI2C_busName I2C bus device name
 * @param devAddr address of the I2C device on the bus
 * @return pointer to the I2C device structure, or INVALID_I2C_BUS, INVALID_I2C_ADDRESS error codes.
 */
i2c_device
i2c_open(const char *pcI2C_busName, uint32_t devAddr)
{
  struct i2c_device_unix *id = malloc(sizeof(struct i2c_device_unix));

  if (id == 0)
    return INVALID_I2C_BUS ;

  id->fd = open(pcI2C_busName, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (id->fd == -1) {
    perror("Cannot open I2C bus");
    i2c_close(id);
    return INVALID_I2C_BUS ;
  }

  if (ioctl(id->fd, I2C_SLAVE, devAddr) < 0) {
    perror("Cannot select I2C device");
    i2c_close(id);
    return INVALID_I2C_ADDRESS ;
  }

  return id;
}

/**
 * @brief Close the I2C device
 *
 * @param id I2C device to close.
 */
void
i2c_close(const i2c_device id)
{
  if (I2C_DATA(id) ->fd >= 0) {
    close(I2C_DATA(id) ->fd);
  }
  free(id);
}

/**
 * @brief Read a frame from the I2C device and copy data to \a pbtRx
 *
 * @param id I2C device.
 * @param pbtRx pointer on buffer used to store data
 * @param szRx length of the buffer
 * @return length (in bytes) of read data, or driver error code  (negative value)
 */
ssize_t
i2c_read(i2c_device id, uint8_t *pbtRx, const size_t szRx)
{
  ssize_t res;
  ssize_t recCount;

  recCount = read(I2C_DATA(id) ->fd, pbtRx, szRx);

  if (recCount < 0) {
    res = NFC_EIO;
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR,
            "Error: read only %d bytes (%d expected) (%s).", (int)recCount, (int) szRx, strerror(errno));
  } else {
    if (recCount < (ssize_t)szRx) {
      res = NFC_EINVARG;
    } else {
      res = recCount;
    }
  }
  return res;
}

/**
 * @brief Write a frame to I2C device containing \a pbtTx content
 *
 * @param id I2C device.
 * @param pbtTx pointer on buffer containing data
 * @param szTx length of the buffer
 * @return NFC_SUCCESS on success, otherwise driver error code
 */
int
i2c_write(i2c_device id, const uint8_t *pbtTx, const size_t szTx)
{
  LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);

  ssize_t writeCount;
  writeCount = write(I2C_DATA(id) ->fd, pbtTx, szTx);

  if ((const ssize_t) szTx == writeCount) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
            "wrote %d bytes successfully.", (int)szTx);
    return NFC_SUCCESS;
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR,
            "Error: wrote only %d bytes (%d expected) (%s).", (int)writeCount, (int) szTx, strerror(errno));
    return NFC_EIO;
  }
}

/**
 * @brief Get the path of all I2C bus devices.
 *
 * @return array of strings defining the names of all I2C buses available. This array, and each string, are allocated
 *     by this function and must be freed by caller.
 */
char **
i2c_list_ports(void)
{
  char **res = malloc(sizeof(char *));
  if (!res) {
    perror("malloc");
    return res;
  }
  size_t szRes = 1;

  res[0] = NULL;
  DIR *pdDir;
  if ((pdDir = opendir("/dev")) == NULL) {
    perror("opendir error: /dev");
    return res;
  }
  struct dirent *pdDirEnt;
  while ((pdDirEnt = readdir(pdDir)) != NULL) {
    const char **p = i2c_ports_device_radix;
    while (*p) {
      if (!strncmp(pdDirEnt->d_name, *p, strlen(*p))) {
        char **res2 = realloc(res, (szRes + 1) * sizeof(char *));
        if (!res2) {
          perror("malloc");
          goto oom;
        }
        res = res2;
        if (!(res[szRes - 1] = malloc(6 + strlen(pdDirEnt->d_name)))) {
          perror("malloc");
          goto oom;
        }
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

