/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2010, Yobibe
 
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

*/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#if defined(HAVE_READLINE)
#  include <readline/readline.h>
#  include <readline/history.h>
#else
   extern FILE* stdin;
#endif //HAVE_READLINE

#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>

#include "nfc-utils.h"

#include "chips/pn53x.h"

#define MAX_FRAME_LEN 264

int main(int argc, const char* argv[])
{
  nfc_device_t* pnd;
  byte_t abtRx[MAX_FRAME_LEN];
  byte_t abtTx[MAX_FRAME_LEN] = { 0xD4 };
  size_t szRxLen;
  size_t szTxLen;

  nfc_device_desc_t device_desc;
  // Try to open the NFC reader
  pnd = nfc_connect(NULL);

  if (pnd == NULL) {
    ERR ("%s", "Unable to connect to NFC device.");
    return EXIT_FAILURE;
  }

  printf ("Connected to NFC reader: %s\n", pnd->acName);

  nfc_initiator_init(pnd);

  char * cmd;
  char * prompt="> ";
  while(1) {
    bool result;
    int offset=0;
#if defined(HAVE_READLINE)
    cmd=readline(prompt);
    // NULL if ctrl-d
    if (cmd==NULL) {
      printf("Bye!\n");
      break;
    }
    add_history(cmd);
#else
    cmd = NULL;
    printf("%s", prompt);
    fflush(0);
    size_t n;
    extern FILE* stdin;
    //FIXME: getline not in stdio.h ???
    int s = getline(&cmd, &n, stdin);
    if (s <= 0) {
      printf("Bye!\n");
      free(cmd);
      break;
    }
    //FIXME: print only if read from redirected stdin (i.e. script)
    printf("%s", cmd);
#endif //HAVE_READLINE
    if (cmd[0]=='q') {
      printf("Bye!\n");
      free(cmd);
      break;
    }
    szTxLen = 0;
    for(int i = 0; i<MAX_FRAME_LEN-10; i++) {
      int size;
      byte_t byte;
      while (isspace(cmd[offset])) {
        offset++;
      }
      size = sscanf(cmd+offset, "%2x", &byte);
      if (size<1) {
        break;
      }
      abtTx[i+1] = byte;
      szTxLen++;
      if (cmd[offset+1] == 0) { // if last hex was only 1 symbol
        break;
      }
      offset += 2;
    }

    if ((int)szTxLen < 1) {
      free(cmd);
      continue;
    }
    szTxLen++;
    printf("Tx: ");
    print_hex((byte_t*)abtTx+1,szTxLen-1);

    // FIXME: Direct call
    if (!pn53x_transceive (pnd, abtTx, szTxLen, abtRx, &szRxLen)) {
      free(cmd);
      nfc_perror (pnd, "Rx");
      continue;
    }

    printf("Rx: ");
    print_hex(abtRx, szRxLen);
    free(cmd);
  }

  nfc_disconnect(pnd);
  return 1;
}
