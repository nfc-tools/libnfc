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
 * Copyright (C) 2014      Pim 't Hart
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
 * @file jewel.h
 * @brief provide samples structs and functions to manipulate Jewel Topaz tags using libnfc
 */

#ifndef _LIBNFC_JEWEL_H_
#  define _LIBNFC_JEWEL_H_

#  include <nfc/nfc-types.h>

// Compiler directive, set struct alignment to 1 uint8_t for compatibility
#  pragma pack(1)

typedef enum {
  TC_RID = 0x78,	// Read ID
  // List of commands (Static memory model)
  TC_RALL = 0x00,	// Real All
  TC_READ = 0x01,	// Read (single byte)
  TC_WRITEE = 0x53,	// Write-with-Erase (single byte)
  TC_WRITENE = 0x1A,	// Write-without-Erase (single byte)
  // List of commands (Dynamic memory model)
  TC_RSEG = 0x10,	// Read segment
  TC_READ8 = 0x02,	// Read (8-bytes)
  TC_WRITEE8 = 0x54,	// Write-with-Erase (8 bytes)
  TC_WRITENE8 = 0x1B	// Write-without-Erase (8 byes)
} jewel_cmd;

// Jewel request
typedef struct {
  uint8_t  btCmd;
} jewel_req_rid;

typedef struct {
  uint8_t  btCmd;
} jewel_req_rall;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd;
} jewel_req_read;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd;
  uint8_t  btDat;
} jewel_req_writee;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd;
  uint8_t  btDat;
} jewel_req_writene;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAddS;
} jewel_req_rseg;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd8;
} jewel_req_read8;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd8;
  uint8_t abtDat[8];
} jewel_req_writee8;

typedef struct {
  uint8_t  btCmd;
  uint8_t  btAdd8;
  uint8_t abtDat[8];
} jewel_req_writene8;

typedef union {
  jewel_req_rid      rid;
  jewel_req_rall     rall;
  jewel_req_read     read;
  jewel_req_writee   writee;
  jewel_req_writene  writene;
  jewel_req_rseg     rseg;
  jewel_req_read8    read8;
  jewel_req_writee8  writee8;
  jewel_req_writene8 writene8;
} jewel_req;

// Jewel responses
typedef struct {
  uint8_t abtHr[2];
  uint8_t abtUid[4];		// 4-LSB from UID
} jewel_res_rid;

typedef struct {
  uint8_t abtHr[2];
  uint8_t abtDat[104];		// Block 0 - E, but not block D (reserved)
} jewel_res_rall;

typedef struct {
  uint8_t  btDat;
} jewel_res_read;

typedef struct {
  uint8_t  btDat;
} jewel_res_writee;

typedef struct {
  uint8_t  btDat;
} jewel_res_writene;

typedef struct {
  uint8_t abtDat[128];
} jewel_res_rseg;

typedef struct {
  uint8_t abtDat[8];
} jewel_res_read8;

typedef struct {
  uint8_t abtDat[8];
} jewel_res_writee8;

typedef struct {
  uint8_t abtDat[8];
} jewel_res_writene8;

typedef union {
  jewel_res_rid      rid;
  jewel_res_rall     rall;
  jewel_res_read     read;
  jewel_res_writee   writee;
  jewel_res_writene  writene;
  jewel_res_rseg     rseg;
  jewel_res_read8    read8;
  jewel_res_writee8  writee8;
  jewel_res_writene8 writene8;
} jewel_res;

// Jewel tag
typedef struct {
  uint8_t abtUid[7];
  uint8_t  btReserved;
} jewel_block_uid;

typedef struct {
  uint8_t abtData[8];
} jewel_block_data;

typedef struct {
  uint8_t abtReserved[8];
} jewel_block_reserved;

typedef struct {
  uint8_t abtLock[2];
  uint8_t abtOtp[6];
} jewel_block_lockotp;

typedef struct {
  jewel_block_uid       bu;
  jewel_block_data     abd[12];
  jewel_block_reserved  br;
  jewel_block_lockotp   bl;
} jewel_tag_blocks;

typedef struct {
  uint8_t abtData[120];
} jewel_tag_data;

typedef union {
  jewel_tag_blocks ttb;
  jewel_tag_data   ttd;
} jewel_tag;

// Reset struct alignment to default
#  pragma pack()

bool nfc_initiator_jewel_cmd(nfc_device *pnd, const jewel_req req, jewel_res *pres);

#endif // _LIBNFC_JEWEL_H_
