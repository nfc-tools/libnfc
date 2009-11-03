/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * 
 * @file messages.h
 * @brief
 */

#ifndef _LIBNFC_MESSAGES_H_
#define _LIBNFC_MESSAGES_H_

// #define DEBUG   /* DEBUG flag can also be enabled using ./configure --enable-debug */

// Useful macros
#ifdef DEBUG
//   #define DBG(x, args...) printf("DBG %s:%d: " x "\n", __FILE__, __LINE__,## args )
  #define DBG(x, ...) fprintf(stderr, "DBG %s:%d: " x "\n", __FILE__, __LINE__, ## __VA_ARGS__ )
#else
  #define DBG(...) {}
#endif

#define INFO(x, ...) printf("INFO: " x "\n", ## __VA_ARGS__ )
#define WARN(x, ...) printf("WARNING: " x "\n", ## __VA_ARGS__ )
#define ERR(x, ...) fprintf(stderr, "ERROR: " x "\n", ## __VA_ARGS__ )

#endif // _LIBNFC_MESSAGES_H_
