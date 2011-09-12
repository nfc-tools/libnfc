/*-
 * Copyright (C) 2011, Romain Tartière, Romuald Conty
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

#include "config.h"

#include <fcntl.h>
#include <log4c.h>
#include <semaphore.h>

#include "log.h"

sem_t *__log_sem;
const char *__sem_name = "/libnfc";

static uint8_t __log_init_counter = 0;

int
log_init (void)
{
  if ((__log_sem = sem_open (__sem_name, O_CREAT, 0666, 1)) == SEM_FAILED) {
    perror ("sem_open");
    return -1;
  }
  int res = 0;

  if (__log_init_counter == 0) {
    res = log4c_init ();
  }
  if (!res) {
    __log_init_counter++;
  }
  return res;
}

int
log_fini (void)
{
  int res = 0;
  if (__log_init_counter >= 1) {
    if (__log_init_counter == 1) {
      sem_close (__log_sem);
      sem_unlink (__sem_name);
      res = log4c_fini ();
    }
    __log_init_counter--;
  } else {
    res = -1;
  }
  return res;
}

void
log_put (char *category, int priority, char *format, ...)
{
  sem_wait (__log_sem);

  const log4c_category_t *cat = log4c_category_get (category);
  if (log4c_category_is_priority_enabled (cat, priority)) {
    va_list va;
    va_start (va, format);
    log4c_category_vlog (cat, priority, format, va);
  }

  sem_post (__log_sem);
}
