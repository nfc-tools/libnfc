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
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file pn53x-tamashell.c
 * @brief Configures the NFC device to communicate with a SAM (Secure Access Module).
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#  include <stdio.h>
#if defined(HAVE_READLINE)
#  include <readline/readline.h>
#  include <readline/history.h>
#endif //HAVE_READLINE

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef _WIN32
#  include <time.h>
#  define msleep(x) do { \
    struct timespec xsleep; \
    xsleep.tv_sec = x / 1000; \
    xsleep.tv_nsec = (x - xsleep.tv_sec * 1000) * 1000 * 1000; \
    nanosleep(&xsleep, NULL); \
  } while (0)
#else
#  include <winbase.h>
#  define msleep Sleep
#endif


#include <nfc/nfc.h>

#include "utils/nfc-utils.h"
#include "libnfc/chips/pn53x.h"

#define MAX_FRAME_LEN 264

int main(int argc, const char *argv[])
{
  nfc_device *pnd;
  uint8_t abtRx[MAX_FRAME_LEN];
  uint8_t abtTx[MAX_FRAME_LEN];
  size_t szRx = sizeof(abtRx);
  size_t szTx;
  FILE *input = NULL;

  if (argc >= 2) {
    if ((input = fopen(argv[1], "r")) == NULL) {
      ERR("%s", "Cannot open file.");
      exit(EXIT_FAILURE);
    }
  }

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  // Try to open the NFC reader
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    if (input != NULL) {
      fclose(input);
    }
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));
  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    if (input != NULL) {
      fclose(input);
    }
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  const char *prompt = "> ";
  while (1) {
    int offset = 0;
    char *cmd;
#if defined(HAVE_READLINE)
    if (input == NULL) { // means we use stdin
      cmd = readline(prompt);
      // NULL if ctrl-d
      if (cmd == NULL) {
        printf("Bye!\n");
        break;
      }
      add_history(cmd);
    } else {
#endif //HAVE_READLINE
      size_t n = 512;
      char *ret = NULL;
      cmd = malloc(n);
      printf("%s", prompt);
      fflush(0);
      if (input != NULL) {
        ret = fgets(cmd, n, input);
      } else {
        ret = fgets(cmd, n, stdin);
      }
      if (ret == NULL || strlen(cmd) <= 0) {
        printf("Bye!\n");
        free(cmd);
        break;
      }
      // FIXME print only if read from redirected stdin (i.e. script)
      printf("%s", cmd);
#if defined(HAVE_READLINE)
    }
#endif //HAVE_READLINE
    if (cmd[0] == 'q') {
      printf("Bye!\n");
      free(cmd);
      break;
    }
    if (cmd[0] == 'p') {
      int ms = 0;
      offset++;
      while (isspace(cmd[offset])) {
        offset++;
      }
      sscanf(cmd + offset, "%10d", &ms);
      printf("Pause for %i msecs\n", ms);
      if (ms > 0) {
        msleep(ms);
      }
      free(cmd);
      continue;
    }
    szTx = 0;
    for (int i = 0; i < MAX_FRAME_LEN; i++) {
      int size;
      unsigned int byte;
      while (isspace(cmd[offset])) {
        offset++;
      }
      size = sscanf(cmd + offset, "%2x", &byte);
      if (size < 1) {
        break;
      }
      abtTx[i] = byte;
      szTx++;
      if (cmd[offset + 1] == 0) { // if last hex was only 1 symbol
        break;
      }
      offset += 2;
    }

    if ((int)szTx < 1) {
      free(cmd);
      continue;
    }
    printf("Tx: ");
    print_hex(abtTx, szTx);

    szRx = sizeof(abtRx);
    int res = 0;
    if ((res = pn53x_transceive(pnd, abtTx, szTx, abtRx, szRx, 0)) < 0) {
      free(cmd);
      nfc_perror(pnd, "Rx");
      continue;
    }
    szRx = (size_t) res;

    printf("Rx: ");
    print_hex(abtRx, szRx);
    free(cmd);
  }

  if (input != NULL) {
    fclose(input);
  }
  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}
