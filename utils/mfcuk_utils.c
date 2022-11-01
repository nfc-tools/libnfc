/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Package version:
    0.1

 Filename:
    mfcuk_utils.c

 Description:
    MFCUK common utility functions implementation.

 License:
    GPL2 (see below), Copyright (C) 2009, Andrei Costin

 * @file mfcuk_utils.c
 * @brief
*/

/*
 VERSION HISTORY
--------------------------------------------------------------------------------
| Number     : 0.1
| dd/mm/yyyy : 23/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Moved bulk of defines and prototypes from "mfcuk_keyrecovery_darkside.c"
--------------------------------------------------------------------------------
*/

/*
 LICENSE

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#if defined(WIN32)
#include <windows.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#else
#error "Unsupported system"
#endif

#include "mfcuk_utils.h"
#include <stdio.h>

/*
http://www.velocityreviews.com/forums/t451319-advice-required-on-my-ascii-to-hex-conversion-c.html
Basically, converting a hex digit into a hex nibble (4 binary digits) algorithm looks like;
        char xdigit; // hex digit to convert [0-9A-Fa-f]
        xdigit = tolower(xdigit); // make it lowercase [0-9a-f]
        xdigit -= '0'; // if it was a [0-9] digit, it's the value now
        if(xdigit > 9) // if it was a [a-f] digit, compensate for that
        xdigit = xdigit + '0' - 'a';
The below code is just an optimization of the algorithm. Maxim Yegorushkin
*/

/*inline*/
int is_hex(char c)
{
  return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'f');
}

/*inline*/
unsigned char hex2bin(unsigned char h, unsigned char l)
{
  h |= 0x20; // to lower
  h -= 0x30;
  h -= -(h > 9) & 0x27;
  l |= 0x20;
  l -= 0x30;
  l -= -(l > 9) & 0x27;
  return h << 4 | l;
}

void sleepmillis(unsigned int millis)
{
#ifdef WIN32 // If system is Windows, use system's own function if possible to reduce overhead, even if a standard C library is available
  Sleep(millis);
#else
  usleep(millis * 1000);
#endif
}

void clear_screen()
{
#ifdef WIN32 // On Windows, use "cls" command
  system("cls");
#else // Otherwise fall back to TTY control characters
  printf("\033[1;1H\033[J");
#endif
}

