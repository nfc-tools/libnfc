/*-
 * Copyright (C) 2012 Romuald Conty
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

#include "conf.h"

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

#define LIBNFC_SYSCONFDIR      "/etc/nfc"
#define LIBNFC_CONFFILE        LIBNFC_SYSCONFDIR"/libnfc.conf"
#define LIBNFC_DEVICECONFDIR   LIBNFC_SYSCONFDIR"/devices.d"

static bool 
conf_parse_file(const char* filename, void (*conf_keyvalue)(void* data, const char* key, const char* value), void* data)
{
  FILE *f = fopen (filename, "r");
  if (!f) {
    perror ("fopen");
    return false;
  }
  char line[BUFSIZ];
  const char *str_regex = "^[[:space:]]*([[:alnum:]_]+)[[:space:]]*=[[:space:]]*(\"(.+)\"|([^[:space:]]+))[[:space:]]*$";
  regex_t preg;
  if(regcomp (&preg, str_regex, REG_EXTENDED|REG_NOTEOL) != 0) {
    printf ("regcomp error\n");
    return false;
  }
  size_t nmatch = preg.re_nsub + 1;
  regmatch_t *pmatch = malloc (sizeof (*pmatch) * nmatch);
  if(!pmatch) {
    perror ("malloc");
    return false;
  }

  int lineno = 0;
  while (fgets(line, BUFSIZ, f) != NULL) {
    lineno++;
    switch(line[0]) {
      case '#':
      case '\n':
        break;
      default: {
        int match;
        if ((match = regexec (&preg, line, nmatch, pmatch, 0)) == 0) {
          const size_t key_size = pmatch[1].rm_eo - pmatch[1].rm_so;
          const off_t  value_pmatch = pmatch[3].rm_eo!=-1?3:4;
          const size_t value_size = pmatch[value_pmatch].rm_eo - pmatch[value_pmatch].rm_so;
          char key[key_size+1];
          char value[value_size+1];
          strncpy(key, line+(pmatch[1].rm_so), key_size); key[key_size]='\0';
          strncpy(value, line+(pmatch[value_pmatch].rm_so), value_size); value[value_size]='\0';
          conf_keyvalue(data, key, value);
        } else {
          log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "parse error on line #%d: %s", lineno, line);
        }
      }
      break;
    }
  }
  return false;
}

static void 
conf_keyvalue_context(void *data, const char* key, const char* value)
{
  nfc_context *context = (nfc_context*)data;
  printf ("key: [%s], value: [%s]\n", key, value);
  if (strcmp(key, "allow_autoscan") == 0) {
    string_as_boolean(value, &(context->allow_autoscan));
  } else if (strcmp(key, "allow_intrusive_scan") == 0) {
    string_as_boolean(value, &(context->allow_intrusive_scan));
  } else if (strcmp(key, "log_level") == 0) {
    context->log_level = atoi(value);
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_INFO, "unknown key in config line: %s = %s", key, value);
  }
}

void 
conf_load(nfc_context *context)
{
  conf_parse_file(LIBNFC_CONFFILE, conf_keyvalue_context, context);
}

/*
int 
main(int argc, char *argv[])
{
  DIR *d = opendir(LIBNFC_DEVICECONFDIR);
  if (!d) {
    perror ("opendir");
  } else {
    struct dirent* de;
    while (de = readdir(d)) {
      if (de->d_name[0]!='.') {
        printf ("\t%s\n", de->d_name);
        char filename[BUFSIZ] = LIBNFC_DEVICECONFDIR"/";
        strcat (filename, de->d_name);
        struct stat s;
        if (stat(filename, &s) == -1) {
          perror("stat");
          continue;
        }
        if(S_ISREG(s.st_mode)) {
          nfc_open_from_file (filename);
        }
      }
    }
  }
}
*/
