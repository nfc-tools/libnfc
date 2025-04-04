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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include <nfc/nfc.h>
#include "nfc-internal.h"
#include "log.h"

#define LOG_CATEGORY "libnfc.config"
#define LOG_GROUP    NFC_LOG_GROUP_CONFIG

#define LIBNFC_CONFFILE      LIBNFC_CONFDIR "/libnfc.conf"
#define LIBNFC_DEVICECONFDIR LIBNFC_CONFDIR "/devices.d"

static int
escaped_value(const char line[BUFSIZ], int i, char **value)
{
  if (line[i] != '"')
    goto FAIL;
  i++;
  if (line[i] == 0 || line[i] == '\n')
    goto FAIL;
  int c = 0;
  while (line[i] && line[i] != '"') {
    i++;
    c++;
  }
  if (line[i] != '"')
    goto FAIL;
  *value = malloc(c + 1);
  if (!*value)
    goto FAIL;
  memset(*value, 0, c + 1);
  memcpy(*value, &line[i - c], c);
  i++;
  while (line[i] && isspace(line[i]))
    i++;
  if (line[i] != 0 && line[i] != '\n')
    goto FAIL;
  return 0;

FAIL:
  free(*value);
  *value = NULL;
  return -1;
}

static int
non_escaped_value(const char line[BUFSIZ], int i, char **value)
{
  int c = 0;
  while (line[i] && !isspace(line[i])) {
    i++;
    c++;
  }
  *value = malloc(c + 1);
  if (!*value)
    goto FAIL;
  memset(*value, 0, c + 1);
  memcpy(*value, &line[i - c], c);
  i++;
  while (line[i] && isspace(line[i]))
    i++;
  if (line[i] != 0)
    goto FAIL;
  return 0;

FAIL:
  free(*value);
  *value = NULL;
  return -1;
}

static int
parse_line(const char line[BUFSIZ], char **key, char **value)
{
  *key = NULL;
  *value = NULL;
  int i = 0;
  int c = 0;

  // optional initial spaces
  while (isspace(line[i]))
    i++;
  if (line[i] == 0 || line[i] == '\n')
    return -1;

  // key
  while (isalnum(line[i]) || line[i] == '_' || line[i] == '.') {
    i++;
    c++;
  }
  if (c == 0 || line[i] == 0 || line[i] == '\n') // key is empty
    return -1;
  *key = malloc(c + 1);
  if (!*key)
    return -1;
  memset(*key, 0, c + 1);
  memcpy(*key, &line[i - c], c);

  // space before '='
  while (isspace(line[i]))
    i++;
  if (line[i] != '=')
    return -1;
  i++;
  if (line[i] == 0 || line[i] == '\n')
    return -1;
  // space after '='
  while (isspace(line[i]))
    i++;
  if (line[i] == 0 || line[i] == '\n')
    return -1;
  if (escaped_value(line, i, value) == 0)
    return 0;
  else if (non_escaped_value(line, i, value) == 0)
    return 0;

  // Extracting key or value failed
  free(*key);
  *key = NULL;
  free(*value);
  *value = NULL;
  return -1;
}

static void
conf_parse_file(const char *filename,
                void (*conf_keyvalue)(void *data, const char *key, const char *value),
                void *data)
{
  FILE *f = fopen(filename, "r");
  if (!f) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "Unable to open file: %s", filename);
    return;
  }
  char line[BUFSIZ];

  int lineno = 0;
  while (fgets(line, BUFSIZ, f) != NULL) {
    lineno++;
    switch (line[0]) {
      case '#':
      case '\n':
        break;
      default: {
        char *key;
        char *value;
        if (parse_line(line, &key, &value) == 0) {
          conf_keyvalue(data, key, value);
          free(key);
          free(value);
        } else {
          free(key);
          free(value);
          log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Parse error on line #%d: %s", lineno, line);
        }
      }
    }
  }
  fclose(f);
  return;
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
    strncpy(context->user_defined_devices[context->user_defined_device_count - 1].name, value, DEVICE_NAME_LENGTH - 1);
    context->user_defined_devices[context->user_defined_device_count - 1].name[DEVICE_NAME_LENGTH - 1] = '\0';
  } else if (strcmp(key, "device.connstring") == 0) {
    if ((context->user_defined_device_count == 0) || strcmp(context->user_defined_devices[context->user_defined_device_count - 1].connstring, "") != 0) {
      if (context->user_defined_device_count >= MAX_USER_DEFINED_DEVICES) {
        log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Configuration exceeded maximum user-defined devices.");
        return;
      }
      context->user_defined_device_count++;
    }
    strncpy(context->user_defined_devices[context->user_defined_device_count - 1].connstring, value, NFC_BUFSIZE_CONNSTRING - 1);
    context->user_defined_devices[context->user_defined_device_count - 1].connstring[NFC_BUFSIZE_CONNSTRING - 1] = '\0';
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
    while ((de =  readdir(d)) != NULL)  {
      // FIXME add a way to sort devices
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
