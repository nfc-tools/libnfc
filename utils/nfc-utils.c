/*-
 * Public platform independent Near Field Communication (NFC) library examples
 * 
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romuald Conty, Romain Tartière
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

#include <nfc/nfc.h>
#include <err.h>

#include "nfc-utils.h"

static const byte_t OddParity[256] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

byte_t
oddparity (const byte_t bt)
{
  return OddParity[bt];
}

void
oddparity_bytes_ts (const byte_t * pbtData, const size_t szLen, byte_t * pbtPar)
{
  size_t  szByteNr;
  // Calculate the parity bits for the command
  for (szByteNr = 0; szByteNr < szLen; szByteNr++) {
    pbtPar[szByteNr] = OddParity[pbtData[szByteNr]];
  }
}

void
print_hex (const byte_t * pbtData, const size_t szBytes)
{
  size_t  szPos;

  for (szPos = 0; szPos < szBytes; szPos++) {
    printf ("%02x  ", pbtData[szPos]);
  }
  printf ("\n");
}

void
print_hex_bits (const byte_t * pbtData, const size_t szBits)
{
  uint8_t uRemainder;
  size_t  szPos;
  size_t  szBytes = szBits / 8;

  for (szPos = 0; szPos < szBytes; szPos++) {
    printf ("%02x  ", pbtData[szPos]);
  }

  uRemainder = szBits % 8;
  // Print the rest bits
  if (uRemainder != 0) {
    if (uRemainder < 5)
      printf ("%01x (%d bits)", pbtData[szBytes], uRemainder);
    else
      printf ("%02x (%d bits)", pbtData[szBytes], uRemainder);
  }
  printf ("\n");
}

void
print_hex_par (const byte_t * pbtData, const size_t szBits, const byte_t * pbtDataPar)
{
  uint8_t uRemainder;
  size_t  szPos;
  size_t  szBytes = szBits / 8;

  for (szPos = 0; szPos < szBytes; szPos++) {
    printf ("%02x", pbtData[szPos]);
    if (OddParity[pbtData[szPos]] != pbtDataPar[szPos]) {
      printf ("! ");
    } else {
      printf ("  ");
    }
  }

  uRemainder = szBits % 8;
  // Print the rest bits, these cannot have parity bit
  if (uRemainder != 0) {
    if (uRemainder < 5)
      printf ("%01x (%d bits)", pbtData[szBytes], uRemainder);
    else
      printf ("%02x (%d bits)", pbtData[szBytes], uRemainder);
  }
  printf ("\n");
}

#define SAK_UID_NOT_COMPLETE     0x04
#define SAK_ISO14443_4_COMPLIANT 0x20
#define SAK_ISO18092_COMPLIANT   0x40

void
print_nfc_iso14443a_info (const nfc_iso14443a_info_t nai, bool verbose)
{
  printf ("    ATQA (SENS_RES): ");
  print_hex (nai.abtAtqa, 2);
  if (verbose) {
    printf("* UID size: ");
    switch ((nai.abtAtqa[1] & 0xc0)>>6) {
      case 0:
        printf("single\n");
      break;
      case 1:
        printf("double\n");
      break;
      case 2:
        printf("triple\n");
      break;
      case 3:
        printf("RFU\n");
      break;
    }
    printf("* bit frame anticollision ");
    switch (nai.abtAtqa[1] & 0x1f) {
      case 0x01:
      case 0x02:
      case 0x04:
      case 0x08:
      case 0x10:
        printf("supported\n");
      break;
      default:
        printf("not supported\n");
      break;
    }
  }
  printf ("       UID (NFCID%c): ", (nai.abtUid[0] == 0x08 ? '3' : '1'));
  print_hex (nai.abtUid, nai.szUidLen);
  if (verbose) {
    if (nai.abtUid[0] == 0x08) {
      printf ("* Random UID\n");
    }
  }
  printf ("      SAK (SEL_RES): ");
  print_hex (&nai.btSak, 1);
  if (verbose) {
    if (nai.btSak & SAK_UID_NOT_COMPLETE) {
      printf ("* Warning! Cascade bit set: UID not complete\n");
    }
    if (nai.btSak & SAK_ISO14443_4_COMPLIANT) {
      printf ("* Compliant with ISO/IEC 14443-4\n");
    } else {
      printf ("* Not compliant with ISO/IEC 14443-4\n");
    }
    if (nai.btSak & SAK_ISO18092_COMPLIANT) {
      printf ("* Compliant with ISO/IEC 18092\n");
    } else {
      printf ("* Not compliant with ISO/IEC 18092\n");
    }
  }
  if (nai.szAtsLen) {
    printf ("                ATS: ");
    print_hex (nai.abtAts, nai.szAtsLen);
  }
  if (nai.szAtsLen && verbose) {
    // Decode ATS according to ISO/IEC 14443-4 (5.2 Answer to select)
    const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
    printf ("* Max Frame Size accepted by PICC: %d bytes\n", iMaxFrameSizes[nai.abtAts[0] & 0x0F]);

    size_t offset = 1;
    if (nai.abtAts[0] & 0x10) { // TA(1) present
      byte_t TA = nai.abtAts[offset];
      offset++;
      printf ("* Bit Rate Capability:\n");
      if (TA == 0) {
        printf ("  * PICC supports only 106 kbits/s in both directions\n");
      }
      if (TA & 1<<7) {
        printf ("  * Same bitrate in both directions mandatory\n");
      }
      if (TA & 1<<4) {
        printf ("  * PICC to PCD, DS=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1<<5) {
        printf ("  * PICC to PCD, DS=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1<<6) {
        printf ("  * PICC to PCD, DS=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1<<0) {
        printf ("  * PCD to PICC, DR=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1<<1) {
        printf ("  * PCD to PICC, DR=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1<<2) {
        printf ("  * PCD to PICC, DR=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1<<3) {
        printf ("  * ERROR unknown value\n");
      }
    }
    if (nai.abtAts[0] & 0x20) { // TB(1) present
      byte_t TB= nai.abtAts[offset];
      offset++;
      printf ("* Frame Waiting Time: %.4g ms\n",256.0*16.0*(1<<((TB & 0xf0) >> 4))/13560.0);
      if ((TB & 0x0f) == 0) {
        printf ("* No Start-up Frame Guard Time required\n");
      } else {
        printf ("* Start-up Frame Guard Time: %.4g ms\n",256.0*16.0*(1<<(TB & 0x0f))/13560.0);
      }
    }
    if (nai.abtAts[0] & 0x40) { // TC(1) present
      byte_t TC = nai.abtAts[offset];
      offset++;
      if (TC & 0x1) {
        printf("* Node ADdress supported\n");
      } else {
        printf("* Node ADdress not supported\n");
      }
      if (TC & 0x2) {
        printf("* Card IDentifier supported\n");
      } else {
        printf("* Card IDentifier not supported\n");
      }
    }
    if (nai.szAtsLen > offset) {
      printf ("* Historical bytes Tk: " );
      print_hex (nai.abtAts + offset, (nai.szAtsLen - offset));
      byte_t CIB = nai.abtAts[offset];
      offset++;
      if (CIB != 0x00 && CIB != 0x10 && (CIB & 0xf0) != 0x80) {
        printf("  * Proprietary format\n");
        if (CIB == 0xc1) {
          printf("    * Tag byte: Mifare or virtual cards of various types\n");
          byte_t L = nai.abtAts[offset];
          offset++;
          if (L != (nai.szAtsLen - offset)) {
            printf("    * Warning: Type Identification Coding length (%i)", L);
            printf(" not matching Tk length (%zi)\n", (nai.szAtsLen - offset));
          }
          if ((nai.szAtsLen - offset - 2) > 0) { // Omit 2 CRC bytes
            byte_t CTC = nai.abtAts[offset];
            offset++;
            printf("    * Chip Type: ");
            switch (CTC & 0xf0) {
              case 0x00:
                printf("(Multiple) Virtual Cards\n");
              break;
              case 0x10:
                printf("Mifare DESFire\n");
              break;
              case 0x20:
                printf("Mifare Plus\n");
              break;
              default:
                printf("RFU\n");
              break;
            }
            printf("    * Memory size: ");
            switch (CTC & 0x0f) {
              case 0x00:
                printf("<1 kbyte\n");
              break;
              case 0x01:
                printf("1 kbyte\n");
              break;
              case 0x02:
                printf("2 kbyte\n");
              break;
              case 0x03:
                printf("4 kbyte\n");
              break;
              case 0x04:
                printf("8 kbyte\n");
              break;
              case 0x0f:
                printf("Unspecified\n");
              break;
              default:
                printf("RFU\n");
              break;
            }
          }
          if ((nai.szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            byte_t CVC = nai.abtAts[offset];
            offset++;
            printf("    * Chip Status: ");
            switch (CVC & 0xf0) {
              case 0x00:
                printf("Engineering sample\n");
              break;
              case 0x20:
                printf("Released\n");
              break;
              default:
                printf("RFU\n");
              break;
            }
            printf("    * Chip Generation: ");
            switch (CVC & 0x0f) {
              case 0x00:
                printf("Generation 1\n");
              break;
              case 0x01:
                printf("Generation 2\n");
              break;
              case 0x02:
                printf("Generation 3\n");
              break;
              case 0x0f:
                printf("Unspecified\n");
              break;
              default:
                printf("RFU\n");
              break;
            }
          }
          if ((nai.szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            byte_t VCS = nai.abtAts[offset];
            offset++;
            printf("    * Specifics (Virtual Card Selection):\n");
            if ((VCS & 0x09) == 0x00) {
              printf("      * Only VCSL supported\n");
            } else if ((VCS & 0x09) == 0x01) {
              printf("      * VCS, VCSL and SVC supported\n");
            }
            if ((VCS & 0x0e) == 0x00) {
              printf("      * SL1, SL2(?), SL3 supported\n");
            } else if ((VCS & 0x0e) == 0x02) {
              printf("      * SL3 only card\n");
            } else if ((VCS & 0x0f) == 0x0e) {
              printf("      * No VCS command supported\n");
            } else if ((VCS & 0x0f) == 0x0f) {
              printf("      * Unspecified\n");
            } else {
              printf("      * RFU\n");
            }
          }
        }
      } else {
        if (CIB == 0x00) {
          printf("  * Tk after 0x00 consist of optional consecutive COMPACT-TLV data objects\n");
          printf("    followed by a mandatory status indicator (the last three bytes, not in TLV)\n");
          printf("    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
        }
        if (CIB == 0x10) {
          printf("  * DIR data reference: %02x\n", nai.abtAts[offset]);
        }
        if (CIB == 0x80) {
          if (nai.szAtsLen == offset) {
            printf("  * No COMPACT-TLV objects found, no status found\n");
          } else {
            printf("  * Tk after 0x80 consist of optional consecutive COMPACT-TLV data objects;\n");
            printf("    the last data object may carry a status indicator of one, two or three bytes.\n");
            printf("    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
          }
        }
      }
    }
  }
  if (verbose) {
    printf("Fingerprinting based on ATQA & SAK values:\n");
    uint32_t atqasak = 0;
    atqasak += (((uint32_t)nai.abtAtqa[0] & 0xff)<<16);
    atqasak += (((uint32_t)nai.abtAtqa[1] & 0xff)<<8);
    atqasak += ((uint32_t)nai.btSak & 0xff);
    bool found_possible_match = false;
    switch (atqasak) {
      case 0x000218:
        printf("* Mifare Classic 4K\n");
        found_possible_match = true;
      break;
      case 0x000408:
        printf("* Mifare Classic 1K\n");
        printf("* Mifare Plus (4-byte UID) 2K SL1\n");
        found_possible_match = true;
      break;
      case 0x000409:
        printf("* Mifare MINI\n");
        found_possible_match = true;
      break;
      case 0x000410:
        printf("* Mifare Plus (4-byte UID) 2K SL2\n");
        found_possible_match = true;
      break;
      case 0x000411:
        printf("* Mifare Plus (4-byte UID) 4K SL2\n");
        found_possible_match = true;
      break;
      case 0x000418:
        printf("* Mifare Plus (4-byte UID) 4K SL1\n");
        found_possible_match = true;
      break;
      case 0x000420:
        printf("* Mifare Plus (4-byte UID) 2K/4K SL3\n");
        found_possible_match = true;
      break;
      case 0x004400:
        printf("* Mifare Ultralight\n");
        printf("* Mifare UltralightC\n");
        found_possible_match = true;
      break;
      case 0x004208:
      case 0x004408:
        printf("* Mifare Plus (7-byte UID) 2K SL1\n");
        found_possible_match = true;
      break;
      case 0x004218:
      case 0x004418:
        printf("* Mifare Plus (7-byte UID) 4K SL1\n");
        found_possible_match = true;
      break;
      case 0x004210:
      case 0x004410:
        printf("* Mifare Plus (7-byte UID) 2K SL2\n");
        found_possible_match = true;
      break;
      case 0x004211:
      case 0x004411:
        printf("* Mifare Plus (7-byte UID) 4K SL2\n");
        found_possible_match = true;
      break;
      case 0x004220:
      case 0x004420:
        printf("* Mifare Plus (7-byte UID) 2K/4K SL3\n");
        found_possible_match = true;
      break;
      case 0x034420:
        printf("* Mifare DESFire / Desfire EV1\n");
        found_possible_match = true;
      break;
    }

    // Other matches not described in
    // AN MIFARE Type Identification Procedure
    // but seen in the field:
    switch (atqasak) {
      case 0x000488:
        printf("* Mifare Classic 1K Infineon\n");
        found_possible_match = true;
      break;
      case 0x000298:
        printf("* Gemplus MPCOS\n");
        found_possible_match = true;
      break;
      case 0x030428:
        printf("* JCOP31\n");
        found_possible_match = true;
      break;
      case 0x004820:
        printf("* JCOP31 v2.4.1\n");
        printf("* JCOP31 v2.2\n");
        found_possible_match = true;
      break;
      case 0x000428:
        printf("* JCOP31 v2.3.1\n");
        found_possible_match = true;
      break;
      case 0x000453:
        printf("* Fudan FM1208SH01\n");
        found_possible_match = true;
      break;
      case 0x000820:
        printf("* Fudan FM1208\n");
        found_possible_match = true;
      break;
      case 0x000238:
        printf("* MFC 4K emulated by Nokia 6212 Classic\n");
        found_possible_match = true;
      break;
      case 0x000838:
        printf("* MFC 4K emulated by Nokia 6131 NFC\n");
        found_possible_match = true;
      break;
    }
    if ((nai.abtAtqa[0] & 0xf0) == 0) {
      switch (nai.abtAtqa[1]) {
        case 0x02:
          printf("* SmartMX with Mifare 4K emulation\n");
          found_possible_match = true;
        break;
        case 0x04:
          printf("* SmartMX with Mifare 1K emulation\n");
          found_possible_match = true;
        break;
        case 0x48:
          printf("* SmartMX with 7-byte UID\n");
          found_possible_match = true;
        break;
      }
    }
    if (! found_possible_match) {
      printf("* Unknown card, sorry\n");
    }
  }
}

void
print_nfc_felica_info (const nfc_felica_info_t nfi, bool verbose)
{
  (void) verbose;
  printf ("        ID (NFCID2): ");
  print_hex (nfi.abtId, 8);
  printf ("    Parameter (PAD): ");
  print_hex (nfi.abtPad, 8);
}

void
print_nfc_jewel_info (const nfc_jewel_info_t nji, bool verbose)
{
  (void) verbose;
  printf ("    ATQA (SENS_RES): ");
  print_hex (nji.btSensRes, 2);
  printf ("      4-LSB JEWELID: ");
  print_hex (nji.btId, 4);
}

#define PI_ISO14443_4_SUPPORTED 0x01
#define PI_NAD_SUPPORTED        0x01
#define PI_CID_SUPPORTED        0x02
void
print_nfc_iso14443b_info (const nfc_iso14443b_info_t nbi, bool verbose)
{
  const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
  printf ("               PUPI: ");
  print_hex (nbi.abtPupi, 4);
  printf ("   Application Data: ");
  print_hex (nbi.abtApplicationData, 4);
  printf ("      Protocol Info: ");
  print_hex (nbi.abtProtocolInfo, 3);
  if (verbose) {
    printf ("* Bit Rate Capability:\n");
    if (nbi.abtProtocolInfo[0] == 0) {
      printf (" * PICC supports only 106 kbits/s in both directions\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<7) {
      printf (" * Same bitrate in both directions mandatory\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<4) {
      printf (" * PICC to PCD, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<5) {
      printf (" * PICC to PCD, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<6) {
      printf (" * PICC to PCD, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<0) {
      printf (" * PCD to PICC, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<1) {
      printf (" * PCD to PICC, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<2) {
      printf (" * PCD to PICC, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1<<3) {
      printf (" * ERROR unknown value\n");
    }
    if( (nbi.abtProtocolInfo[1] & 0xf0) <= 0x80 ) {
      printf ("* Maximum frame sizes: %d bytes\n", iMaxFrameSizes[((nbi.abtProtocolInfo[1] & 0xf0) >> 4)]);
    }
    if((nbi.abtProtocolInfo[1] & 0x0f) == PI_ISO14443_4_SUPPORTED) {
      printf ("* Protocol types supported: ISO/IEC 14443-4\n");
    }
    printf ("* Frame Waiting Time: %.4g ms\n",256.0*16.0*(1<<((nbi.abtProtocolInfo[2] & 0xf0) >> 4))/13560.0);
    if((nbi.abtProtocolInfo[2] & (PI_NAD_SUPPORTED|PI_CID_SUPPORTED)) != 0) {
      printf ("* Frame options supported: ");
      if ((nbi.abtProtocolInfo[2] & PI_NAD_SUPPORTED) != 0) printf ("NAD ");
      if ((nbi.abtProtocolInfo[2] & PI_CID_SUPPORTED) != 0) printf ("CID ");
      printf("\n");
    }
  }
}

void
print_nfc_iso14443bi_info (const nfc_iso14443bi_info_t nii, bool verbose)
{
  printf ("                DIV: ");
  print_hex (nii.abtDIV, 4);
  if (verbose) {
    int version = (nii.btVerLog & 0x1e)>>1;
    printf ("   Software Version: ");
    if (version == 15) {
      printf ("Undefined\n");
    } else {
      printf ("%i\n", version);
    }

    if ((nii.btVerLog & 0x80) && (nii.btConfig & 0x80)){
      printf ("        Wait Enable: yes");
    }
  }
  if ((nii.btVerLog & 0x80) && (nii.btConfig & 0x40)) {
    printf ("                ATS: ");
    print_hex (nii.abtAtr, nii.szAtrLen);
  }
}

void
print_nfc_iso14443b2sr_info (const nfc_iso14443b2sr_info_t nsi, bool verbose)
{
  (void) verbose;
  printf ("                UID: ");
  print_hex (nsi.abtUID, 8);
}

void
print_nfc_iso14443b2ct_info (const nfc_iso14443b2ct_info_t nci, bool verbose)
{
  (void) verbose;
  uint32_t uid;
  uid = (nci.abtUID[3] << 24) + (nci.abtUID[2] << 16) + (nci.abtUID[1] << 8) + nci.abtUID[0];
  printf ("                UID: ");
  print_hex (nci.abtUID, sizeof(nci.abtUID));
  printf ("      UID (decimal): %010u\n", uid);
  printf ("       Product Code: %02X\n", nci.btProdCode);
  printf ("           Fab Code: %02X\n", nci.btFabCode);
}

void
print_nfc_dep_info (const nfc_dep_info_t ndi, bool verbose)
{
  (void) verbose;
  printf ("       NFCID3: ");
  print_hex (ndi.abtNFCID3, 10);
  printf ("           BS: %02x\n", ndi.btBS);
  printf ("           BR: %02x\n", ndi.btBR);
  printf ("           TO: %02x\n", ndi.btTO);
  printf ("           PP: %02x\n", ndi.btPP);
  if (ndi.szGB) {
    printf ("General Bytes: ");
    print_hex (ndi.abtGB, ndi.szGB);
  }
}

/**
 * @brief Tries to parse arguments to find device descriptions.
 * @return Returns the list of found device descriptions.
 */
nfc_device_desc_t *
parse_args (int argc, const char *argv[], size_t * szFound, bool * verbose)
{
  nfc_device_desc_t *pndd = 0;
  int     arg;
  *szFound = 0;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {

    if (0 == strcmp (argv[arg], "--device")) {
      // FIXME: this device selection by command line options is terrible & does not support USB/PCSC drivers
      if (argc > arg + 1) {
        char    buffer[256];

        pndd = malloc (sizeof (nfc_device_desc_t));

        strncpy (buffer, argv[++arg], 256);

        // Driver.
        pndd->pcDriver = (char *) malloc (256);
        strcpy (pndd->pcDriver, strtok (buffer, ":"));

        // Port.
        strcpy (pndd->acPort, strtok (NULL, ":"));

        // Speed.
        sscanf (strtok (NULL, ":"), "%u", &pndd->uiSpeed);

        *szFound = 1;
      } else {
        errx (1, "usage: %s [--device driver:port:speed]", argv[0]);
      }
    }
    if ((0 == strcmp (argv[arg], "-v")) || (0 == strcmp (argv[arg], "--verbose"))) {
      *verbose = true;
    }
  }
  return pndd;
}

const char *
str_nfc_baud_rate (const nfc_baud_rate_t nbr)
{
  switch(nbr) {
    case NBR_UNDEFINED:
      return "undefined baud rate";
    break;
    case NBR_106:
      return "106 kbps";
    break;
    case NBR_212:
      return "212 kbps";
    break;
    case NBR_424:
      return "424 kbps";
    break;
    case NBR_847:
      return "847 kbps";
    break;
  }
  return "";
}

void
print_nfc_target (const nfc_target_t nt, bool verbose)
{
  switch(nt.nm.nmt) {
    case NMT_ISO14443A:
      printf ("ISO/IEC 14443A (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_iso14443a_info (nt.nti.nai, verbose);
    break;
    case NMT_JEWEL:
      printf ("Innovision Jewel (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_jewel_info (nt.nti.nji, verbose);
    break;
    case NMT_FELICA:
      printf ("FeliCa (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_felica_info (nt.nti.nfi, verbose);
    break;
    case NMT_ISO14443B:
      printf ("ISO/IEC 14443-4B (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_iso14443b_info (nt.nti.nbi, verbose);
    break;
    case NMT_ISO14443BI:
      printf ("ISO/IEC 14443-4B' (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_iso14443bi_info (nt.nti.nii, verbose);
    break;
    case NMT_ISO14443B2SR:
      printf ("ISO/IEC 14443-2B ST SRx (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_iso14443b2sr_info (nt.nti.nsi, verbose);
    break;
    case NMT_ISO14443B2CT:
      printf ("ISO/IEC 14443-2B ASK CTx (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_iso14443b2ct_info (nt.nti.nci, verbose);
    break;
    case NMT_DEP:
      printf ("D.E.P. (%s) target:\n", str_nfc_baud_rate(nt.nm.nbr));
      print_nfc_dep_info (nt.nti.ndi, verbose);
    break;
  }
}

