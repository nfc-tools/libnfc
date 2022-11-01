/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Package version:
    0.1

 Filename:
    mfcuk_mifare.h

 Description:
    MFCUK defines and function prototypes header file extending
    mainly libnfc's "mifare.h" interface/functionality.

 Contact, bug-reports:
    http://andreicostin.com/
    mailto:zveriu@gmail.com

 License:
    GPL2 (see below), Copyright (C) 2009, Andrei Costin

 * @file mfcuk_mifare.h
 * @brief
*/

/*
 VERSION HISTORY
--------------------------------------------------------------------------------
| Number     : 0.1
| dd/mm/yyyy : 23/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Moved bulk of defines and functions from "mfcuk_keyrecovery_darkside.c"
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

#ifndef _MFCUK_MIFARE_H_
#define _MFCUK_MIFARE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <nfc/nfc.h>
#include "mifare.h"

#define MIFARE_CLASSIC_UID_BYTELENGTH           4       // Length of a Mifare Classic UID in bytes
#define MIFARE_CLASSIC_KEY_BYTELENGTH           6       // Length of a Mifare Classic key in bytes
#define MIFARE_CLASSIC_1K_NAME                  "MC1K"
#define MIFARE_CLASSIC_4K_NAME                  "MC4K"
#define MIFARE_CLASSIC_UNKN_NAME                "UNKN"
#define MIFARE_CLASSIC_1K                       0x08    // MF1ICS50 Functional Specifications - 0x08
#define MIFARE_CLASSIC_4K                       0x18    // MF1ICS70 Functional Specifications - 0x18
#define MIFARE_DESFIRE                          0x20    // XXXXXXXX Functional Specifications - 0x20
#define MIFARE_CLASSIC_1K_RATB                  0x88    // Infineon Licensed Mifare 1K = 0x88 (thanks JPS)
#define MIFARE_CLASSIC_4K_SKGT                  0x98    // Infineon Licensed Mifare 4K = 0x98???

#define IS_MIFARE_CLASSIC_1K(ats_sak)           ( ((ats_sak) == MIFARE_CLASSIC_1K) || ((ats_sak) == MIFARE_CLASSIC_1K_RATB) )
#define IS_MIFARE_CLASSIC_4K(ats_sak)           ( ((ats_sak) == MIFARE_CLASSIC_4K) || ((ats_sak) == MIFARE_CLASSIC_4K_SKGT) )
#define IS_MIFARE_DESFIRE(ats_sak)              ( ((ats_sak) == MIFARE_DESFIRE) )

#define IS_MIFARE_CLASSIC_1K_TAG(tag)           IS_MIFARE_CLASSIC_1K(tag->amb[0].mbm.btSAK)
#define IS_MIFARE_CLASSIC_4K_TAG(tag)           IS_MIFARE_CLASSIC_4K(tag->amb[0].mbm.btSAK)
#define IS_MIFARE_DESFIRE_TAG(tag)              IS_MIFARE_DESFIRE(tag->amb[0].mbm.btSAK)

#define MIFARE_CLASSIC_BYTES_PER_BLOCK          16 // Common for Mifare Classic 1K and Mifare Classic 4K
#define MIFARE_CLASSIC_INVALID_BLOCK            0xFFFFFFFF

#define MIFARE_CLASSIC_1K_MAX_SECTORS           16
#define MIFARE_CLASSIC_1K_BLOCKS_PER_SECTOR     4
#define MIFARE_CLASSIC_1K_MAX_BLOCKS            ( (MIFARE_CLASSIC_1K_MAX_SECTORS) * (MIFARE_CLASSIC_1K_BLOCKS_PER_SECTOR) )

#define MIFARE_CLASSIC_4K_MAX_SECTORS1          32
#define MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1    MIFARE_CLASSIC_1K_BLOCKS_PER_SECTOR // Possibly NXP made it for Mifare 1K backward compatibility
#define MIFARE_CLASSIC_4K_MAX_BLOCKS1           ( (MIFARE_CLASSIC_4K_MAX_SECTORS1) * (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) )

#define MIFARE_CLASSIC_4K_MAX_SECTORS2          8
#define MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2    16
#define MIFARE_CLASSIC_4K_MAX_BLOCKS2           ( (MIFARE_CLASSIC_4K_MAX_SECTORS2) * (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) )

#define MIFARE_CLASSIC_4K_MAX_SECTORS           ( (MIFARE_CLASSIC_4K_MAX_SECTORS1) + (MIFARE_CLASSIC_4K_MAX_SECTORS2) )
#define MIFARE_CLASSIC_4K_MAX_BLOCKS            ( (MIFARE_CLASSIC_4K_MAX_BLOCKS1) + (MIFARE_CLASSIC_4K_MAX_BLOCKS2) )

#define MFCUK_EXTENDED_DESCRIPTION_LENGTH       128

// Define an extended type of dump, basically a wrapper dump around basic tag dump
typedef struct {
    uint32_t uid;  // looks redundant, but it is easier to use dmp.uid instead of dmp.amb.mbm.abtUID[0]...[3]
    uint8_t type; // ATS/SAK from ti.tia.btSak, example 0x08h for Mifare 1K, 0x18h for Mifare 4K
    char datetime[14]; // non-zero-terminated date-time of dump in format YYYYMMDDH24MISS, example 20091114231541 - 14 Nov 2009, 11:15:41 PM
    char description[MFCUK_EXTENDED_DESCRIPTION_LENGTH]; // a description of the tag dump, example "RATB_DUMP_BEFORE_PAY"
    mifare_classic_tag tag_basic;
} mifare_classic_tag_ext;

// Define type of keys (A or B) in NXP notation
typedef enum {
    keyA = 0x60,
    keyB = 0x61,
} mifare_key_type;

// Default keys used as a *BIG* mistake in many applications - especially System Integrators should pay attention!
extern uint8_t mfcuk_default_keys[][MIFARE_CLASSIC_KEY_BYTELENGTH];
extern int mfcuk_default_keys_num;

bool is_valid_block(uint8_t bTagType, uint32_t uiBlock);
bool is_valid_sector(uint8_t bTagType, uint32_t uiSector);
bool is_first_block(uint8_t bTagType, uint32_t uiBlock);
bool is_trailer_block(uint8_t bTagType, uint32_t uiBlock);
uint32_t get_first_block(uint8_t bTagType, uint32_t uiBlock);
uint32_t get_trailer_block(uint8_t bTagType, uint32_t uiBlock);
bool is_big_sector(uint8_t bTagType, uint32_t uiSector);
uint32_t get_first_block_for_sector(uint8_t bTagType, uint32_t uiSector);
uint32_t get_trailer_block_for_sector(uint8_t bTagType, uint32_t uiSector);
uint32_t get_sector_for_block(uint8_t bTagType, uint32_t uiBlock);
bool is_first_sector(uint8_t bTagType, uint32_t uiSector);
bool is_first_big_sector(uint8_t bTagType, uint32_t uiSector);
bool is_first_small_sector(uint8_t bTagType, uint32_t uiSector);
bool is_last_sector(uint8_t bTagType, uint32_t uiSector);
bool is_last_big_sector(uint8_t bTagType, uint32_t uiSector);
bool is_last_small_sector(uint8_t bTagType, uint32_t uiSector);
void test_mifare_classic_blocks_sectors_functions(uint8_t bTagType);
bool mfcuk_save_tag_dump(const char* filename, mifare_classic_tag* tag);
bool mfcuk_save_tag_dump_ext(const char* filename, mifare_classic_tag_ext* tag_ext);
bool mfcuk_load_tag_dump(const char* filename, mifare_classic_tag* tag);
bool mfcuk_load_tag_dump_ext(const char* filename, mifare_classic_tag_ext* tag_ext);
void print_mifare_classic_tag_keys(const char* title, mifare_classic_tag* tag);
bool mfcuk_key_uint64_to_arr(const uint64_t* ui64Key, uint8_t* arr6Key);
bool mfcuk_key_arr_to_uint64(const uint8_t* arr6Key, uint64_t* ui64Key);

#endif // _MFCUK_MIFARE_H_
