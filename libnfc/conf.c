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
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "conf.h"

#ifdef CONFFILES
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>

#include <nfc/nfc.h>
#include "nfc-internal.h"
#include "log.h"

#define LOG_CATEGORY "libnfc.config"
#define LOG_GROUP    NFC_LOG_GROUP_CONFIG

#ifndef LIBNFC_SYSCONFDIR
// If this define does not already exists, we build it using SYSCONFDIR
#ifndef SYSCONFDIR
#error "SYSCONFDIR is not defined but required."
#endif // SYSCONFDIR
#define LIBNFC_SYSCONFDIR      SYSCONFDIR"/nfc"
#endif // LIBNFC_SYSCONFDIR

#define LIBNFC_CONFFILE        LIBNFC_SYSCONFDIR"/libnfc.conf"
#define LIBNFC_DEVICECONFDIR   LIBNFC_SYSCONFDIR"/devices.d"

static bool
conf_parse_file(const char *filename, void (*conf_keyvalue)(void *data, const char *key, const char *value), void *data)
{
  FILE *f = fopen(filename, "r");
  if (!f) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "Unable to open file: %s", filename);
    return false;
  }
  char line[BUFSIZ];
  const char *str_regex = "^[[:space:]]*([[:alnum:]_.]+)[[:space:]]*=[[:space:]]*(\"(.+)\"|([^[:space:]]+))[[:space:]]*$";
  regex_t preg;
  if (regcomp(&preg, str_regex, REG_EXTENDED | REG_NOTEOL) != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Regular expression used for configuration file parsing is not valid.");
    fclose(f);
    return false;
  }
  size_t nmatch = preg.re_nsub + 1;
  regmatch_t *pmatch = malloc(sizeof(*pmatch) * nmatch);
  if (!pmatch) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Not enough memory: malloc failed.");
    regfree(&preg);
    fclose(f);
    return false;
  }

  int lineno = 0;
  while (fgets(line, BUFSIZ, f) != NULL) {
    lineno++;
    switch (line[0]) {
      case '#':
      case '\n':
        break;
      default: {
        int match;
        if ((match = regexec(&preg, line, nmatch, pmatch, 0)) == 0) {
          const size_t key_size = pmatch[1].rm_eo - pmatch[1].rm_so;
          const off_t  value_pmatch = pmatch[3].rm_eo != -1 ? 3 : 4;
          const size_t value_size = pmatch[value_pmatch].rm_eo - pmatch[value_pmatch].rm_so;
          char key[key_size + 1];
          char value[value_size + 1];
          strncpy(key, line + (pmatch[1].rm_so), key_size);
          key[key_size] = '\0';
          strncpy(value, line + (pmatch[value_pmatch].rm_so), value_size);
          value[value_size] = '\0';
          conf_keyvalue(data, key, value);
        } else {
          log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Parse error on line #%d: %s", lineno, line);
        }
      }
      break;
    }
  }

  free(pmatch);
  regfree(&preg);
  fclose(f);
  return false;
}

static void
conf_keyvalue_context(void *data, const char *key, const char *value)
{
  nfc_context *context = (nfc_context *)data;
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "key: [%s], value: [%s]", key, value);
  if (strcmp(key, "allow_autoscan") == 0) {
    string_as_boolean(value, &(context->allow_autoscan));
  } else if (strcmp(key, "allow_intrusive_scan") == 0) {
    string_as_boolean(value, &(context->allow_intrusive_scan));
  } else if (strcmp(key, "log_level") == 0) {
    context->log_level = atoi(value);
  } else if (strcmp(key, "device.name") == 0) {
    if ((context->user_defined_device_count == 0) || strcmp(context->user_defined_devices[context->user_defined_device_count - 1].name, "") != 0) {
      if (context->user_defined_device_count >= MAX_USER_DEFINED_DEVICES) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Configuration exceeded maximum user-defined devices.");
        return;
      }
      context->user_defined_device_count++;
    }
    strcpy(context->user_defined_devices[context->user_defined_device_count - 1].name, value);
  } else if (strcmp(key, "device.connstring") == 0) {
    if ((context->user_defined_device_count == 0) || strcmp(context->user_defined_devices[context->user_defined_device_count - 1].connstring, "") != 0) {
      if (context->user_defined_device_count >= MAX_USER_DEFINED_DEVICES) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Configuration exceeded maximum user-defined devices.");
        return;
      }
      context->user_defined_device_count++;
    }
    strcpy(context->user_defined_devices[context->user_defined_device_count - 1].connstring, value);
  } else if (strcmp(key, "device.optional") == 0) {
    if ((context->user_defined_device_count == 0) || context->user_defined_devices[context->user_defined_device_count - 1].optional) {
      if (context->user_defined_device_count >= MAX_USER_DEFINED_DEVICES) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Configuration exceeded maximum user-defined devices.");
        return;
      }
      context->user_defined_device_count++;
    }
    if ((strcmp(value, "true") == 0) || (strcmp(value, "True") == 0) || (strcmp(value, "1") == 0)) //optional
      context->user_defined_devices[context->user_defined_device_count - 1].optional = true;
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "Unknown key in config line: %s = %s", key, value);
  }
}

static void
conf_keyvalue_device(void *data, const char *key, const char *value)
{
  char newkey[BUFSIZ];
  sprintf(newkey, "device.%s", key);
  conf_keyvalue_context(data, newkey, value);
}

static void
conf_devices_load(const char *dirname, nfc_context *context)
{
  DIR *d = opendir(dirname);
  if (!d) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Unable to open directory: %s", dirname);
  } else {
    struct dirent *de;
    struct dirent entry;
    struct dirent *result;
    while ((readdir_r(d, &entry, &result) == 0) && (result != NULL)) {
      de = &entry;
      if (de->d_name[0] != '.') {
        const size_t filename_len = strlen(de->d_name);
        const size_t extension_len = strlen(".conf");
        if ((filename_len > extension_len) &&
            (strncmp(".conf", de->d_name + (filename_len - extension_len), extension_len) == 0)) {
          char filename[BUFSIZ] = LIBNFC_DEVICECONFDIR"/";
          strcat(filename, de->d_name);
          struct stat s;
          if (stat(filename, &s) == -1) {
            perror("stat");
            continue;
          }
          if (S_ISREG(s.st_mode)) {
            conf_parse_file(filename, conf_keyvalue_device, context);
          }
        }
      }
    }
    closedir(d);
  }
}

void
conf_load(nfc_context *context)
{
  conf_parse_file(LIBNFC_CONFFILE, conf_keyvalue_context, context);
  conf_devices_load(LIBNFC_DEVICECONFDIR, context);
}

#endif // CONFFILES

