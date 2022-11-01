/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Filename:
    mfcuk_keyrecovery_darkside.h

 Description:
    MFCUK DarkSide Key Recovery specific typedefs and defines

 Contact, bug-reports:
    http://andreicostin.com/
    mailto:zveriu@gmail.com

 License:
    GPL2 (see below), Copyright (C) 2009, Andrei Costin

 * @file mfcuk_keyrecovery_darkside.h
 * @brief
*/

/*
 VERSION HISTORY
--------------------------------------------------------------------------------
| Number     : 0.1
| dd/mm/yyyy : 23/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Moved bulk of defines and things from "mfcuk_keyrecovery_darkside.c"
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

#ifndef _MFCUK_KEYRECOVERY_DARKSIDE_H_
#define _MFCUK_KEYRECOVERY_DARKSIDE_H_

// Define package and executable related info
#define BUILD_NAME      "Mifare Classic DarkSide Key Recovery Tool"
#define BUILD_VERSION   "0.3"
#define BUILD_AUTHOR    "Andrei Costin, zveriu@gmail.com, http://andreicostin.com"

// Define return statuses
#define MFCUK_SUCCESS                 0x0
#define MFCUK_OK_KEY_RECOVERED        (MFCUK_SUCCESS+1)
#define MFCUK_FAIL_AUTH               (MFCUK_OK_KEY_RECOVERED+1)
#define MFCUK_FAIL_CRAPTO             (MFCUK_FAIL_AUTH+1)
#define MFCUK_FAIL_TAGTYPE_INVALID    (MFCUK_FAIL_CRAPTO+1)
#define MFCUK_FAIL_KEYTYPE_INVALID    (MFCUK_FAIL_TAGTYPE_INVALID+1)
#define MFCUK_FAIL_BLOCK_INVALID      (MFCUK_FAIL_KEYTYPE_INVALID+1)
#define MFCUK_FAIL_SECTOR_INVALID     (MFCUK_FAIL_BLOCK_INVALID+1)
#define MFCUK_FAIL_COMM               (MFCUK_FAIL_SECTOR_INVALID+1)
#define MFCUK_FAIL_MEMORY             (MFCUK_FAIL_COMM+1)

// There are 4 bytes in ACBITS, use each byte as below
#define ACTIONS_KEY_A           0 // Specifies the byte index where actions for key A are stored
#define RESULTS_KEY_A           1 // Specifies the byte index where results for key A are stored
#define ACTIONS_KEY_B           2 // Specifies the byte index where actions for key B are stored
#define RESULTS_KEY_B           3 // Specifies the byte index where results for key B are stored

// The action/result byte can contain any combination of these
#define ACTIONS_VERIFY          0x1 // Specifies whether the key should be first verified
#define ACTIONS_RECOVER         0x2 // Specifies whether the key should be recovered. If a key has verify action and the key was verified, RESULTS_ byte will indicate that and recovery will not take place
#define ACTIONS_KEYSET          0x4 // Specifies whether the key was set from command line rather that should be loaded from the eventual -i/-I dump

#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 16
// Implementation specific, since we are not 100% sure we can fix the tag nonce
// Suppose from 2^32, only MAX 2^16 tag nonces will appear given current SLEEP_ values
#define MAX_TAG_NONCES                  65536
// Maximum possible states allocated and returned by lsfr_common_prefix(). Used this value in the looping
#define MAX_COMMON_PREFIX_STATES        (1<<20)
// 10 ms, though {WPMCC09} claims 30 us is enough
#define SLEEP_AT_FIELD_OFF              10
// 50 ms, seems pretty good constant, though if you don't like it - make it even 3.1415..., we don't care
#define SLEEP_AFTER_FIELD_ON            50
// Since the 29 bits of {Nr} are constant, darkside varies only "last" (0xFFFFFF1F) 3 bits, thus we have 8 possible parity bits arrays
#define MFCUK_DARKSIDE_MAX_LEVELS       8

#define MFCUK_DARKSIDE_START_NR         0xDEADBEEF
#define MFCUK_DARKSIDE_START_AR         0xFACECAFE

typedef struct tag_nonce_entry {
  uint32_t tagNonce; // Tag nonce we target for fixation
  uint8_t spoofFlag; // No spoofing until we have a successful auth with this tagNonce. Once we have, we want to spoof to get the encrypted 0x5 value
  uint32_t num_of_appearances; // For statistics, how many times this tag nonce appeared for the given SLEEP_ values

  // STAGE1 data for "dark side" and lsfr_common_prefix()
  uint32_t spoofNrPfx; // PARAM: used as pfx, calculated from (spoofNrEnc & 0xFFFFFF1F). BUG: weird way to denote "first 29 prefix bits" in "dark side" paper. Perhaps I see the world different
  uint32_t spoofNrEnc; // {Nr} value which we will be using to make the tag respond with 4 bits
  uint32_t spoofArEnc; // PARAM: used as rr
  uint8_t spoofParBitsEnc; // parity bits we are trying to guess for the first time
  uint8_t spoofNackEnc; // store here the encrypted NACK returned first time we match the parity bits
  uint8_t spoofKs; // store here the keystream ks used for encryptying spoofNackEnc, specifically spoofKs = spoofNackEnc ^ 0x5

  // STAGE2 data for "dark side" and lsfr_common_prefix()
  int current_out_of_8; // starting from -1 until we find parity for chosen spoofNrEnc,spoofArEnc
  uint8_t parBitsCrntCombination[MFCUK_DARKSIDE_MAX_LEVELS]; // Loops over 32 combinations of the last 5 parity bits which generated the 4 bit NACK in STAGE1
  uint32_t nrEnc[MFCUK_DARKSIDE_MAX_LEVELS]; // the 29 bits constant prefix, varying only 3 bits, thus 8 possible values
  uint32_t arEnc[MFCUK_DARKSIDE_MAX_LEVELS]; // the same reader response as spoofArEnc; redundant but... :)
  uint8_t ks[MFCUK_DARKSIDE_MAX_LEVELS]; // PARAM: used as ks, obtained as (ks[i] = nackEnc[i] ^ 0x5)
  uint8_t nackEnc[MFCUK_DARKSIDE_MAX_LEVELS]; // store here the encrypted 4 bits values which tag responded
  uint8_t parBits[MFCUK_DARKSIDE_MAX_LEVELS]; // store here the values based on spoofParBitsEnc, varying only last 5 bits
  uint8_t parBitsArr[MFCUK_DARKSIDE_MAX_LEVELS][8]; // PARAM: used as par, contains value of parBits byte-bit values just splitted out one bit per byte thus second pair of braces [8]
} tag_nonce_entry_t;

#endif // _MFCUK_KEYRECOVERY_DARKSIDE_H_
