/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * Additional contributors of this file:
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

/**
 * @file target-subr.c
 * @brief Target-related subroutines. (ie. determine target type, print target, etc.)
 */
#include <inttypes.h>
#include <nfc/nfc.h>

#include "target-subr.h"

struct card_atqa {
  uint16_t atqa;
  uint16_t mask;
  char type[128];
  // list of up to 8 SAK values compatible with this ATQA
  int saklist[8];
};

struct card_sak {
  uint8_t sak;
  uint8_t mask;
  char type[128];
};

struct card_atqa const_ca[] = {
  {
    0x0044, 0xffff, "MIFARE Ultralight",
    {0, -1}
  },
  {
    0x0044, 0xffff, "MIFARE Ultralight C",
    {0, -1}
  },
  {
    0x0004, 0xff0f, "MIFARE Mini 0.3K",
    {1, -1}
  },
  {
    0x0004, 0xff0f, "MIFARE Classic 1K",
    {2, -1}
  },
  {
    0x0002, 0xff0f, "MIFARE Classic 4K",
    {3, -1}
  },
  {
    0x0004, 0xffff, "MIFARE Plus (4 Byte UID or 4 Byte RID)",
    {4, 5, 6, 7, 8, 9, -1}
  },
  {
    0x0002, 0xffff, "MIFARE Plus (4 Byte UID or 4 Byte RID)",
    {4, 5, 6, 7, 8, 9, -1}
  },
  {
    0x0044, 0xffff, "MIFARE Plus (7 Byte UID)",
    {4, 5, 6, 7, 8, 9, -1}
  },
  {
    0x0042, 0xffff, "MIFARE Plus (7 Byte UID)",
    {4, 5, 6, 7, 8, 9, -1}
  },
  {
    0x0344, 0xffff, "MIFARE DESFire",
    {10, 11, -1}
  },
  {
    0x0044, 0xffff, "P3SR008",
    { -1}
  }, // TODO we need SAK info
  {
    0x0004, 0xf0ff, "SmartMX with MIFARE 1K emulation",
    {12, -1}
  },
  {
    0x0002, 0xf0ff, "SmartMX with MIFARE 4K emulation",
    {12, -1}
  },
  {
    0x0048, 0xf0ff, "SmartMX with 7 Byte UID",
    {12, -1}
  }
};

struct card_sak const_cs[] = {
  {0x00, 0xff, "" },                      // 00 MIFARE Ultralight / Ultralight C
  {0x09, 0xff, "" },                      // 01 MIFARE Mini 0.3K
  {0x08, 0xff, "" },                      // 02 MIFARE Classic 1K
  {0x18, 0xff, "" },                      // 03 MIFARE Classik 4K
  {0x08, 0xff, " 2K, Security level 1" }, // 04 MIFARE Plus
  {0x18, 0xff, " 4K, Security level 1" }, // 05 MIFARE Plus
  {0x10, 0xff, " 2K, Security level 2" }, // 06 MIFARE Plus
  {0x11, 0xff, " 4K, Security level 2" }, // 07 MIFARE Plus
  {0x20, 0xff, " 2K, Security level 3" }, // 08 MIFARE Plus
  {0x20, 0xff, " 4K, Security level 3" }, // 09 MIFARE Plus
  {0x20, 0xff, " 4K" },                   // 10 MIFARE DESFire
  {0x20, 0xff, " EV1 2K/4K/8K" },         // 11 MIFARE DESFire
  {0x00, 0x00, "" },                      // 12 SmartMX
};

int
snprint_hex(char *dst, size_t size, const uint8_t *pbtData, const size_t szBytes)
{
  size_t  szPos;
  size_t res = 0;
  for (szPos = 0; szPos < szBytes; szPos++) {
    res += snprintf(dst + res, size - res, "%02x  ", pbtData[szPos]);
  }
  res += snprintf(dst + res, size - res, "\n");
  return res;
}

#define SAK_UID_NOT_COMPLETE     0x04
#define SAK_ISO14443_4_COMPLIANT 0x20
#define SAK_ISO18092_COMPLIANT   0x40

void
snprint_nfc_iso14443a_info(char *dst, size_t size, const nfc_iso14443a_info *pnai, bool verbose)
{
  int off = 0;
  off += snprintf(dst + off, size - off, "    ATQA (SENS_RES): ");
  off += snprint_hex(dst + off, size - off, pnai->abtAtqa, 2);
  if (verbose) {
    off += snprintf(dst + off, size - off, "* UID size: ");
    switch ((pnai->abtAtqa[1] & 0xc0) >> 6) {
      case 0:
        off += snprintf(dst + off, size - off, "single\n");
        break;
      case 1:
        off += snprintf(dst + off, size - off, "double\n");
        break;
      case 2:
        off += snprintf(dst + off, size - off, "triple\n");
        break;
      case 3:
        off += snprintf(dst + off, size - off, "RFU\n");
        break;
    }
    off += snprintf(dst + off, size - off, "* bit frame anticollision ");
    switch (pnai->abtAtqa[1] & 0x1f) {
      case 0x01:
      case 0x02:
      case 0x04:
      case 0x08:
      case 0x10:
        off += snprintf(dst + off, size - off, "supported\n");
        break;
      default:
        off += snprintf(dst + off, size - off, "not supported\n");
        break;
    }
  }
  off += snprintf(dst + off, size - off, "       UID (NFCID%c): ", (pnai->abtUid[0] == 0x08 ? '3' : '1'));
  off += snprint_hex(dst + off, size - off, pnai->abtUid, pnai->szUidLen);
  if (verbose) {
    if (pnai->abtUid[0] == 0x08) {
      off += snprintf(dst + off, size - off, "* Random UID\n");
    }
  }
  off += snprintf(dst + off, size - off, "      SAK (SEL_RES): ");
  off += snprint_hex(dst + off, size - off, &pnai->btSak, 1);
  if (verbose) {
    if (pnai->btSak & SAK_UID_NOT_COMPLETE) {
      off += snprintf(dst + off, size - off, "* Warning! Cascade bit set: UID not complete\n");
    }
    if (pnai->btSak & SAK_ISO14443_4_COMPLIANT) {
      off += snprintf(dst + off, size - off, "* Compliant with ISO/IEC 14443-4\n");
    } else {
      off += snprintf(dst + off, size - off, "* Not compliant with ISO/IEC 14443-4\n");
    }
    if (pnai->btSak & SAK_ISO18092_COMPLIANT) {
      off += snprintf(dst + off, size - off, "* Compliant with ISO/IEC 18092\n");
    } else {
      off += snprintf(dst + off, size - off, "* Not compliant with ISO/IEC 18092\n");
    }
  }
  if (pnai->szAtsLen) {
    off += snprintf(dst + off, size - off, "                ATS: ");
    off += snprint_hex(dst + off, size - off, pnai->abtAts, pnai->szAtsLen);
  }
  if (pnai->szAtsLen && verbose) {
    // Decode ATS according to ISO/IEC 14443-4 (5.2 Answer to select)
    const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
    off += snprintf(dst + off, size - off, "* Max Frame Size accepted by PICC: %d bytes\n", iMaxFrameSizes[pnai->abtAts[0] & 0x0F]);

    size_t offset = 1;
    if (pnai->abtAts[0] & 0x10) { // TA(1) present
      uint8_t TA = pnai->abtAts[offset];
      offset++;
      off += snprintf(dst + off, size - off, "* Bit Rate Capability:\n");
      if (TA == 0) {
        off += snprintf(dst + off, size - off, "  * PICC supports only 106 kbits/s in both directions\n");
      }
      if (TA & 1 << 7) {
        off += snprintf(dst + off, size - off, "  * Same bitrate in both directions mandatory\n");
      }
      if (TA & 1 << 4) {
        off += snprintf(dst + off, size - off, "  * PICC to PCD, DS=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1 << 5) {
        off += snprintf(dst + off, size - off, "  * PICC to PCD, DS=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1 << 6) {
        off += snprintf(dst + off, size - off, "  * PICC to PCD, DS=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1 << 0) {
        off += snprintf(dst + off, size - off, "  * PCD to PICC, DR=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1 << 1) {
        off += snprintf(dst + off, size - off, "  * PCD to PICC, DR=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1 << 2) {
        off += snprintf(dst + off, size - off, "  * PCD to PICC, DR=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1 << 3) {
        off += snprintf(dst + off, size - off, "  * ERROR unknown value\n");
      }
    }
    if (pnai->abtAts[0] & 0x20) { // TB(1) present
      uint8_t TB = pnai->abtAts[offset];
      offset++;
      off += snprintf(dst + off, size - off, "* Frame Waiting Time: %.4g ms\n", 256.0 * 16.0 * (1 << ((TB & 0xf0) >> 4)) / 13560.0);
      if ((TB & 0x0f) == 0) {
        off += snprintf(dst + off, size - off, "* No Start-up Frame Guard Time required\n");
      } else {
        off += snprintf(dst + off, size - off, "* Start-up Frame Guard Time: %.4g ms\n", 256.0 * 16.0 * (1 << (TB & 0x0f)) / 13560.0);
      }
    }
    if (pnai->abtAts[0] & 0x40) { // TC(1) present
      uint8_t TC = pnai->abtAts[offset];
      offset++;
      if (TC & 0x1) {
        off += snprintf(dst + off, size - off, "* Node Address supported\n");
      } else {
        off += snprintf(dst + off, size - off, "* Node Address not supported\n");
      }
      if (TC & 0x2) {
        off += snprintf(dst + off, size - off, "* Card IDentifier supported\n");
      } else {
        off += snprintf(dst + off, size - off, "* Card IDentifier not supported\n");
      }
    }
    if (pnai->szAtsLen > offset) {
      off += snprintf(dst + off, size - off, "* Historical bytes Tk: ");
      off += snprint_hex(dst + off, size - off, pnai->abtAts + offset, (pnai->szAtsLen - offset));
      uint8_t CIB = pnai->abtAts[offset];
      offset++;
      if (CIB != 0x00 && CIB != 0x10 && (CIB & 0xf0) != 0x80) {
        off += snprintf(dst + off, size - off, "  * Proprietary format\n");
        if (CIB == 0xc1) {
          off += snprintf(dst + off, size - off, "    * Tag byte: Mifare or virtual cards of various types\n");
          uint8_t L = pnai->abtAts[offset];
          offset++;
          if (L != (pnai->szAtsLen - offset)) {
            off += snprintf(dst + off, size - off, "    * Warning: Type Identification Coding length (%i)", L);
            off += snprintf(dst + off, size - off, " not matching Tk length (%" PRIdPTR ")\n", (pnai->szAtsLen - offset));
          }
          if ((pnai->szAtsLen - offset - 2) > 0) { // Omit 2 CRC bytes
            uint8_t CTC = pnai->abtAts[offset];
            offset++;
            off += snprintf(dst + off, size - off, "    * Chip Type: ");
            switch (CTC & 0xf0) {
              case 0x00:
                off += snprintf(dst + off, size - off, "(Multiple) Virtual Cards\n");
                break;
              case 0x10:
                off += snprintf(dst + off, size - off, "Mifare DESFire\n");
                break;
              case 0x20:
                off += snprintf(dst + off, size - off, "Mifare Plus\n");
                break;
              default:
                off += snprintf(dst + off, size - off, "RFU\n");
                break;
            }
            off += snprintf(dst + off, size - off, "    * Memory size: ");
            switch (CTC & 0x0f) {
              case 0x00:
                off += snprintf(dst + off, size - off, "<1 kbyte\n");
                break;
              case 0x01:
                off += snprintf(dst + off, size - off, "1 kbyte\n");
                break;
              case 0x02:
                off += snprintf(dst + off, size - off, "2 kbyte\n");
                break;
              case 0x03:
                off += snprintf(dst + off, size - off, "4 kbyte\n");
                break;
              case 0x04:
                off += snprintf(dst + off, size - off, "8 kbyte\n");
                break;
              case 0x0f:
                off += snprintf(dst + off, size - off, "Unspecified\n");
                break;
              default:
                off += snprintf(dst + off, size - off, "RFU\n");
                break;
            }
          }
          if ((pnai->szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            uint8_t CVC = pnai->abtAts[offset];
            offset++;
            off += snprintf(dst + off, size - off, "    * Chip Status: ");
            switch (CVC & 0xf0) {
              case 0x00:
                off += snprintf(dst + off, size - off, "Engineering sample\n");
                break;
              case 0x20:
                off += snprintf(dst + off, size - off, "Released\n");
                break;
              default:
                off += snprintf(dst + off, size - off, "RFU\n");
                break;
            }
            off += snprintf(dst + off, size - off, "    * Chip Generation: ");
            switch (CVC & 0x0f) {
              case 0x00:
                off += snprintf(dst + off, size - off, "Generation 1\n");
                break;
              case 0x01:
                off += snprintf(dst + off, size - off, "Generation 2\n");
                break;
              case 0x02:
                off += snprintf(dst + off, size - off, "Generation 3\n");
                break;
              case 0x0f:
                off += snprintf(dst + off, size - off, "Unspecified\n");
                break;
              default:
                off += snprintf(dst + off, size - off, "RFU\n");
                break;
            }
          }
          if ((pnai->szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            uint8_t VCS = pnai->abtAts[offset];
            offset++;
            off += snprintf(dst + off, size - off, "    * Specifics (Virtual Card Selection):\n");
            if ((VCS & 0x09) == 0x00) {
              off += snprintf(dst + off, size - off, "      * Only VCSL supported\n");
            } else if ((VCS & 0x09) == 0x01) {
              off += snprintf(dst + off, size - off, "      * VCS, VCSL and SVC supported\n");
            }
            if ((VCS & 0x0e) == 0x00) {
              off += snprintf(dst + off, size - off, "      * SL1, SL2(?), SL3 supported\n");
            } else if ((VCS & 0x0e) == 0x02) {
              off += snprintf(dst + off, size - off, "      * SL3 only card\n");
            } else if ((VCS & 0x0f) == 0x0e) {
              off += snprintf(dst + off, size - off, "      * No VCS command supported\n");
            } else if ((VCS & 0x0f) == 0x0f) {
              off += snprintf(dst + off, size - off, "      * Unspecified\n");
            } else {
              off += snprintf(dst + off, size - off, "      * RFU\n");
            }
          }
        }
      } else {
        if (CIB == 0x00) {
          off += snprintf(dst + off, size - off, "  * Tk after 0x00 consist of optional consecutive COMPACT-TLV data objects\n");
          off += snprintf(dst + off, size - off, "    followed by a mandatory status indicator (the last three bytes, not in TLV)\n");
          off += snprintf(dst + off, size - off, "    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
        }
        if (CIB == 0x10) {
          off += snprintf(dst + off, size - off, "  * DIR data reference: %02x\n", pnai->abtAts[offset]);
        }
        if (CIB == 0x80) {
          if (pnai->szAtsLen == offset) {
            off += snprintf(dst + off, size - off, "  * No COMPACT-TLV objects found, no status found\n");
          } else {
            off += snprintf(dst + off, size - off, "  * Tk after 0x80 consist of optional consecutive COMPACT-TLV data objects;\n");
            off += snprintf(dst + off, size - off, "    the last data object may carry a status indicator of one, two or three bytes.\n");
            off += snprintf(dst + off, size - off, "    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
          }
        }
      }
    }
  }
  if (verbose) {
    off += snprintf(dst + off, size - off, "\nFingerprinting based on MIFARE type Identification Procedure:\n"); // AN10833
    uint16_t atqa = 0;
    uint8_t sak = 0;
    uint8_t i, j;
    bool found_possible_match = false;

    atqa = (((uint16_t)pnai->abtAtqa[0] & 0xff) << 8);
    atqa += (((uint16_t)pnai->abtAtqa[1] & 0xff));
    sak = ((uint8_t)pnai->btSak & 0xff);

    for (i = 0; i < sizeof(const_ca) / sizeof(const_ca[0]); i++) {
      if ((atqa & const_ca[i].mask) == const_ca[i].atqa) {
        for (j = 0; (j < sizeof(const_ca[i].saklist)) && (const_ca[i].saklist[j] >= 0); j++) {
          int sakindex = const_ca[i].saklist[j];
          if ((sak & const_cs[sakindex].mask) == const_cs[sakindex].sak) {
            off += snprintf(dst + off, size - off, "* %s%s\n", const_ca[i].type, const_cs[sakindex].type);
            found_possible_match = true;
          }
        }
      }
    }
    // Other matches not described in
    // AN10833 MIFARE Type Identification Procedure
    // but seen in the field:
    off += snprintf(dst + off, size - off, "Other possible matches based on ATQA & SAK values:\n");
    uint32_t atqasak = 0;
    atqasak += (((uint32_t)pnai->abtAtqa[0] & 0xff) << 16);
    atqasak += (((uint32_t)pnai->abtAtqa[1] & 0xff) << 8);
    atqasak += ((uint32_t)pnai->btSak & 0xff);
    switch (atqasak) {
      case 0x000488:
        off += snprintf(dst + off, size - off, "* Mifare Classic 1K Infineon\n");
        found_possible_match = true;
        break;
      case 0x000298:
        off += snprintf(dst + off, size - off, "* Gemplus MPCOS\n");
        found_possible_match = true;
        break;
      case 0x030428:
        off += snprintf(dst + off, size - off, "* JCOP31\n");
        found_possible_match = true;
        break;
      case 0x004820:
        off += snprintf(dst + off, size - off, "* JCOP31 v2.4.1\n");
        off += snprintf(dst + off, size - off, "* JCOP31 v2.2\n");
        found_possible_match = true;
        break;
      case 0x000428:
        off += snprintf(dst + off, size - off, "* JCOP31 v2.3.1\n");
        found_possible_match = true;
        break;
      case 0x000453:
        off += snprintf(dst + off, size - off, "* Fudan FM1208SH01\n");
        found_possible_match = true;
        break;
      case 0x000820:
        off += snprintf(dst + off, size - off, "* Fudan FM1208\n");
        found_possible_match = true;
        break;
      case 0x000238:
        off += snprintf(dst + off, size - off, "* MFC 4K emulated by Nokia 6212 Classic\n");
        found_possible_match = true;
        break;
      case 0x000838:
        off += snprintf(dst + off, size - off, "* MFC 4K emulated by Nokia 6131 NFC\n");
        found_possible_match = true;
        break;
    }
    if (! found_possible_match) {
      snprintf(dst + off, size - off, "* Unknown card, sorry\n");
    }
  }
}

void
snprint_nfc_felica_info(char *dst, size_t size, const nfc_felica_info *pnfi, bool verbose)
{
  (void) verbose;
  int off = 0;
  off += snprintf(dst + off, size - off, "        ID (NFCID2): ");
  off += snprint_hex(dst + off, size - off, pnfi->abtId, 8);
  off += snprintf(dst + off, size - off, "    Parameter (PAD): ");
  off += snprint_hex(dst + off, size - off, pnfi->abtPad, 8);
  off += snprintf(dst + off, size - off, "   System Code (SC): ");
  snprint_hex(dst + off, size - off, pnfi->abtSysCode, 2);
}

void
snprint_nfc_jewel_info(char *dst, size_t size, const nfc_jewel_info *pnji, bool verbose)
{
  (void) verbose;
  int off = 0;
  off += snprintf(dst + off, size - off, "    ATQA (SENS_RES): ");
  off += snprint_hex(dst + off, size - off, pnji->btSensRes, 2);
  off += snprintf(dst + off, size - off, "      4-LSB JEWELID: ");
  snprint_hex(dst + off, size - off, pnji->btId, 4);
}

#define PI_ISO14443_4_SUPPORTED 0x01
#define PI_NAD_SUPPORTED        0x01
#define PI_CID_SUPPORTED        0x02
void
snprint_nfc_iso14443b_info(char *dst, size_t size, const nfc_iso14443b_info *pnbi, bool verbose)
{
  int off = 0;
  off += snprintf(dst + off, size - off, "               PUPI: ");
  off += snprint_hex(dst + off, size - off, pnbi->abtPupi, 4);
  off += snprintf(dst + off, size - off, "   Application Data: ");
  off += snprint_hex(dst + off, size - off, pnbi->abtApplicationData, 4);
  off += snprintf(dst + off, size - off, "      Protocol Info: ");
  off += snprint_hex(dst + off, size - off, pnbi->abtProtocolInfo, 3);
  if (verbose) {
    off += snprintf(dst + off, size - off, "* Bit Rate Capability:\n");
    if (pnbi->abtProtocolInfo[0] == 0) {
      off += snprintf(dst + off, size - off, " * PICC supports only 106 kbits/s in both directions\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 7) {
      off += snprintf(dst + off, size - off, " * Same bitrate in both directions mandatory\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 4) {
      off += snprintf(dst + off, size - off, " * PICC to PCD, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 5) {
      off += snprintf(dst + off, size - off, " * PICC to PCD, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 6) {
      off += snprintf(dst + off, size - off, " * PICC to PCD, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 0) {
      off += snprintf(dst + off, size - off, " * PCD to PICC, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 1) {
      off += snprintf(dst + off, size - off, " * PCD to PICC, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 2) {
      off += snprintf(dst + off, size - off, " * PCD to PICC, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (pnbi->abtProtocolInfo[0] & 1 << 3) {
      off += snprintf(dst + off, size - off, " * ERROR unknown value\n");
    }
    if ((pnbi->abtProtocolInfo[1] & 0xf0) <= 0x80) {
      const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
      off += snprintf(dst + off, size - off, "* Maximum frame sizes: %d bytes\n", iMaxFrameSizes[((pnbi->abtProtocolInfo[1] & 0xf0) >> 4)]);
    }
    if ((pnbi->abtProtocolInfo[1] & 0x0f) == PI_ISO14443_4_SUPPORTED) {
      off += snprintf(dst + off, size - off, "* Protocol types supported: ISO/IEC 14443-4\n");
    }
    off += snprintf(dst + off, size - off, "* Frame Waiting Time: %.4g ms\n", 256.0 * 16.0 * (1 << ((pnbi->abtProtocolInfo[2] & 0xf0) >> 4)) / 13560.0);
    if ((pnbi->abtProtocolInfo[2] & (PI_NAD_SUPPORTED | PI_CID_SUPPORTED)) != 0) {
      off += snprintf(dst + off, size - off, "* Frame options supported: ");
      if ((pnbi->abtProtocolInfo[2] & PI_NAD_SUPPORTED) != 0) off += snprintf(dst + off, size - off, "NAD ");
      if ((pnbi->abtProtocolInfo[2] & PI_CID_SUPPORTED) != 0) off += snprintf(dst + off, size - off, "CID ");
      snprintf(dst + off, size - off, "\n");
    }
  }
}

void
snprint_nfc_iso14443bi_info(char *dst, size_t size, const nfc_iso14443bi_info *pnii, bool verbose)
{
  int off = 0;
  off += snprintf(dst + off, size - off, "                DIV: ");
  off += snprint_hex(dst + off, size - off, pnii->abtDIV, 4);
  if (verbose) {
    int version = (pnii->btVerLog & 0x1e) >> 1;
    off += snprintf(dst + off, size - off, "   Software Version: ");
    if (version == 15) {
      off += snprintf(dst + off, size - off, "Undefined\n");
    } else {
      off += snprintf(dst + off, size - off, "%i\n", version);
    }

    if ((pnii->btVerLog & 0x80) && (pnii->btConfig & 0x80)) {
      off += snprintf(dst + off, size - off, "        Wait Enable: yes");
    }
  }
  if ((pnii->btVerLog & 0x80) && (pnii->btConfig & 0x40)) {
    off += snprintf(dst + off, size - off, "                ATS: ");
    snprint_hex(dst + off, size - off, pnii->abtAtr, pnii->szAtrLen);
  }
}

void
snprint_nfc_iso14443b2sr_info(char *dst, size_t size, const nfc_iso14443b2sr_info *pnsi, bool verbose)
{
  (void) verbose;
  int off = 0;
  off += snprintf(dst + off, size - off, "                UID: ");
  snprint_hex(dst + off, size - off, pnsi->abtUID, 8);
}

void
snprint_nfc_iso14443b2ct_info(char *dst, size_t size, const nfc_iso14443b2ct_info *pnci, bool verbose)
{
  (void) verbose;
  int off = 0;
  uint32_t uid;
  uid = (pnci->abtUID[3] << 24) + (pnci->abtUID[2] << 16) + (pnci->abtUID[1] << 8) + pnci->abtUID[0];
  off += snprintf(dst + off, size - off, "                UID: ");
  off += snprint_hex(dst + off, size - off, pnci->abtUID, sizeof(pnci->abtUID));
  off += snprintf(dst + off, size - off, "      UID (decimal): %010u\n", uid);
  off += snprintf(dst + off, size - off, "       Product Code: %02X\n", pnci->btProdCode);
  snprintf(dst + off, size - off, "           Fab Code: %02X\n", pnci->btFabCode);
}

void
snprint_nfc_dep_info(char *dst, size_t size, const nfc_dep_info *pndi, bool verbose)
{
  (void) verbose;
  int off = 0;
  off += snprintf(dst + off, size - off, "       NFCID3: ");
  off += snprint_hex(dst + off, size - off, pndi->abtNFCID3, 10);
  off += snprintf(dst + off, size - off, "           BS: %02x\n", pndi->btBS);
  off += snprintf(dst + off, size - off, "           BR: %02x\n", pndi->btBR);
  off += snprintf(dst + off, size - off, "           TO: %02x\n", pndi->btTO);
  off += snprintf(dst + off, size - off, "           PP: %02x\n", pndi->btPP);
  if (pndi->szGB) {
    off += snprintf(dst + off, size - off, "General Bytes: ");
    snprint_hex(dst + off, size - off, pndi->abtGB, pndi->szGB);
  }
}

void
snprint_nfc_target(char *dst, size_t size, const nfc_target *pnt, bool verbose)
{
  if (NULL != pnt) {
    int off = 0;
    off += snprintf(dst + off, size - off, "%s (%s%s) target:\n", str_nfc_modulation_type(pnt->nm.nmt), str_nfc_baud_rate(pnt->nm.nbr), (pnt->nm.nmt != NMT_DEP) ? "" : (pnt->nti.ndi.ndm == NDM_ACTIVE) ? "active mode" : "passive mode");
    switch (pnt->nm.nmt) {
      case NMT_ISO14443A:
        snprint_nfc_iso14443a_info(dst + off, size - off, &pnt->nti.nai, verbose);
        break;
      case NMT_JEWEL:
        snprint_nfc_jewel_info(dst + off, size - off, &pnt->nti.nji, verbose);
        break;
      case NMT_FELICA:
        snprint_nfc_felica_info(dst + off, size - off, &pnt->nti.nfi, verbose);
        break;
      case NMT_ISO14443B:
        snprint_nfc_iso14443b_info(dst + off, size - off, &pnt->nti.nbi, verbose);
        break;
      case NMT_ISO14443BI:
        snprint_nfc_iso14443bi_info(dst + off, size - off, &pnt->nti.nii, verbose);
        break;
      case NMT_ISO14443B2SR:
        snprint_nfc_iso14443b2sr_info(dst + off, size - off, &pnt->nti.nsi, verbose);
        break;
      case NMT_ISO14443B2CT:
        snprint_nfc_iso14443b2ct_info(dst + off, size - off, &pnt->nti.nci, verbose);
        break;
      case NMT_DEP:
        snprint_nfc_dep_info(dst + off, size - off, &pnt->nti.ndi, verbose);
        break;
    }
  }
}

