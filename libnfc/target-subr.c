/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010-2011, Romain Tartière
 * Copyright (C) 2009-2012, Romuald Conty
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
sprint_hex(char *dst, const uint8_t *pbtData, const size_t szBytes)
{
  size_t  szPos;

  int res = 0;
  for (szPos = 0; szPos < szBytes; szPos++) {
    res += sprintf(dst + res, "%02x  ", pbtData[szPos]);
  }
  res += sprintf(dst + res, "\n");
  return res;
}

#define SAK_UID_NOT_COMPLETE     0x04
#define SAK_ISO14443_4_COMPLIANT 0x20
#define SAK_ISO18092_COMPLIANT   0x40

void
sprint_nfc_iso14443a_info(char *dst, const nfc_iso14443a_info nai, bool verbose)
{
  dst += sprintf(dst, "    ATQA (SENS_RES): ");
  dst += sprint_hex(dst, nai.abtAtqa, 2);
  if (verbose) {
    dst += sprintf(dst, "* UID size: ");
    switch ((nai.abtAtqa[1] & 0xc0) >> 6) {
      case 0:
        dst += sprintf(dst, "single\n");
        break;
      case 1:
        dst += sprintf(dst, "double\n");
        break;
      case 2:
        dst += sprintf(dst, "triple\n");
        break;
      case 3:
        dst += sprintf(dst, "RFU\n");
        break;
    }
    dst += sprintf(dst, "* bit frame anticollision ");
    switch (nai.abtAtqa[1] & 0x1f) {
      case 0x01:
      case 0x02:
      case 0x04:
      case 0x08:
      case 0x10:
        dst += sprintf(dst, "supported\n");
        break;
      default:
        dst += sprintf(dst, "not supported\n");
        break;
    }
  }
  dst += sprintf(dst, "       UID (NFCID%c): ", (nai.abtUid[0] == 0x08 ? '3' : '1'));
  dst += sprint_hex(dst, nai.abtUid, nai.szUidLen);
  if (verbose) {
    if (nai.abtUid[0] == 0x08) {
      dst += sprintf(dst, "* Random UID\n");
    }
  }
  dst += sprintf(dst, "      SAK (SEL_RES): ");
  dst += sprint_hex(dst, &nai.btSak, 1);
  if (verbose) {
    if (nai.btSak & SAK_UID_NOT_COMPLETE) {
      dst += sprintf(dst, "* Warning! Cascade bit set: UID not complete\n");
    }
    if (nai.btSak & SAK_ISO14443_4_COMPLIANT) {
      dst += sprintf(dst, "* Compliant with ISO/IEC 14443-4\n");
    } else {
      dst += sprintf(dst, "* Not compliant with ISO/IEC 14443-4\n");
    }
    if (nai.btSak & SAK_ISO18092_COMPLIANT) {
      dst += sprintf(dst, "* Compliant with ISO/IEC 18092\n");
    } else {
      dst += sprintf(dst, "* Not compliant with ISO/IEC 18092\n");
    }
  }
  if (nai.szAtsLen) {
    dst += sprintf(dst, "                ATS: ");
    dst += sprint_hex(dst, nai.abtAts, nai.szAtsLen);
  }
  if (nai.szAtsLen && verbose) {
    // Decode ATS according to ISO/IEC 14443-4 (5.2 Answer to select)
    const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
    dst += sprintf(dst, "* Max Frame Size accepted by PICC: %d bytes\n", iMaxFrameSizes[nai.abtAts[0] & 0x0F]);

    size_t offset = 1;
    if (nai.abtAts[0] & 0x10) { // TA(1) present
      uint8_t TA = nai.abtAts[offset];
      offset++;
      dst += sprintf(dst, "* Bit Rate Capability:\n");
      if (TA == 0) {
        dst += sprintf(dst, "  * PICC supports only 106 kbits/s in both directions\n");
      }
      if (TA & 1 << 7) {
        dst += sprintf(dst, "  * Same bitrate in both directions mandatory\n");
      }
      if (TA & 1 << 4) {
        dst += sprintf(dst, "  * PICC to PCD, DS=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1 << 5) {
        dst += sprintf(dst, "  * PICC to PCD, DS=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1 << 6) {
        dst += sprintf(dst, "  * PICC to PCD, DS=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1 << 0) {
        dst += sprintf(dst, "  * PCD to PICC, DR=2, bitrate 212 kbits/s supported\n");
      }
      if (TA & 1 << 1) {
        dst += sprintf(dst, "  * PCD to PICC, DR=4, bitrate 424 kbits/s supported\n");
      }
      if (TA & 1 << 2) {
        dst += sprintf(dst, "  * PCD to PICC, DR=8, bitrate 847 kbits/s supported\n");
      }
      if (TA & 1 << 3) {
        dst += sprintf(dst, "  * ERROR unknown value\n");
      }
    }
    if (nai.abtAts[0] & 0x20) { // TB(1) present
      uint8_t TB = nai.abtAts[offset];
      offset++;
      dst += sprintf(dst, "* Frame Waiting Time: %.4g ms\n", 256.0 * 16.0 * (1 << ((TB & 0xf0) >> 4)) / 13560.0);
      if ((TB & 0x0f) == 0) {
        dst += sprintf(dst, "* No Start-up Frame Guard Time required\n");
      } else {
        dst += sprintf(dst, "* Start-up Frame Guard Time: %.4g ms\n", 256.0 * 16.0 * (1 << (TB & 0x0f)) / 13560.0);
      }
    }
    if (nai.abtAts[0] & 0x40) { // TC(1) present
      uint8_t TC = nai.abtAts[offset];
      offset++;
      if (TC & 0x1) {
        dst += sprintf(dst, "* Node ADdress supported\n");
      } else {
        dst += sprintf(dst, "* Node ADdress not supported\n");
      }
      if (TC & 0x2) {
        dst += sprintf(dst, "* Card IDentifier supported\n");
      } else {
        dst += sprintf(dst, "* Card IDentifier not supported\n");
      }
    }
    if (nai.szAtsLen > offset) {
      dst += sprintf(dst, "* Historical bytes Tk: ");
      dst += sprint_hex(dst, nai.abtAts + offset, (nai.szAtsLen - offset));
      uint8_t CIB = nai.abtAts[offset];
      offset++;
      if (CIB != 0x00 && CIB != 0x10 && (CIB & 0xf0) != 0x80) {
        dst += sprintf(dst, "  * Proprietary format\n");
        if (CIB == 0xc1) {
          dst += sprintf(dst, "    * Tag byte: Mifare or virtual cards of various types\n");
          uint8_t L = nai.abtAts[offset];
          offset++;
          if (L != (nai.szAtsLen - offset)) {
            dst += sprintf(dst, "    * Warning: Type Identification Coding length (%i)", L);
            dst += sprintf(dst, " not matching Tk length (%zi)\n", (nai.szAtsLen - offset));
          }
          if ((nai.szAtsLen - offset - 2) > 0) { // Omit 2 CRC bytes
            uint8_t CTC = nai.abtAts[offset];
            offset++;
            dst += sprintf(dst, "    * Chip Type: ");
            switch (CTC & 0xf0) {
              case 0x00:
                dst += sprintf(dst, "(Multiple) Virtual Cards\n");
                break;
              case 0x10:
                dst += sprintf(dst, "Mifare DESFire\n");
                break;
              case 0x20:
                dst += sprintf(dst, "Mifare Plus\n");
                break;
              default:
                dst += sprintf(dst, "RFU\n");
                break;
            }
            dst += sprintf(dst, "    * Memory size: ");
            switch (CTC & 0x0f) {
              case 0x00:
                dst += sprintf(dst, "<1 kbyte\n");
                break;
              case 0x01:
                dst += sprintf(dst, "1 kbyte\n");
                break;
              case 0x02:
                dst += sprintf(dst, "2 kbyte\n");
                break;
              case 0x03:
                dst += sprintf(dst, "4 kbyte\n");
                break;
              case 0x04:
                dst += sprintf(dst, "8 kbyte\n");
                break;
              case 0x0f:
                dst += sprintf(dst, "Unspecified\n");
                break;
              default:
                dst += sprintf(dst, "RFU\n");
                break;
            }
          }
          if ((nai.szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            uint8_t CVC = nai.abtAts[offset];
            offset++;
            dst += sprintf(dst, "    * Chip Status: ");
            switch (CVC & 0xf0) {
              case 0x00:
                dst += sprintf(dst, "Engineering sample\n");
                break;
              case 0x20:
                dst += sprintf(dst, "Released\n");
                break;
              default:
                dst += sprintf(dst, "RFU\n");
                break;
            }
            dst += sprintf(dst, "    * Chip Generation: ");
            switch (CVC & 0x0f) {
              case 0x00:
                dst += sprintf(dst, "Generation 1\n");
                break;
              case 0x01:
                dst += sprintf(dst, "Generation 2\n");
                break;
              case 0x02:
                dst += sprintf(dst, "Generation 3\n");
                break;
              case 0x0f:
                dst += sprintf(dst, "Unspecified\n");
                break;
              default:
                dst += sprintf(dst, "RFU\n");
                break;
            }
          }
          if ((nai.szAtsLen - offset) > 0) { // Omit 2 CRC bytes
            uint8_t VCS = nai.abtAts[offset];
            offset++;
            dst += sprintf(dst, "    * Specifics (Virtual Card Selection):\n");
            if ((VCS & 0x09) == 0x00) {
              dst += sprintf(dst, "      * Only VCSL supported\n");
            } else if ((VCS & 0x09) == 0x01) {
              dst += sprintf(dst, "      * VCS, VCSL and SVC supported\n");
            }
            if ((VCS & 0x0e) == 0x00) {
              dst += sprintf(dst, "      * SL1, SL2(?), SL3 supported\n");
            } else if ((VCS & 0x0e) == 0x02) {
              dst += sprintf(dst, "      * SL3 only card\n");
            } else if ((VCS & 0x0f) == 0x0e) {
              dst += sprintf(dst, "      * No VCS command supported\n");
            } else if ((VCS & 0x0f) == 0x0f) {
              dst += sprintf(dst, "      * Unspecified\n");
            } else {
              dst += sprintf(dst, "      * RFU\n");
            }
          }
        }
      } else {
        if (CIB == 0x00) {
          dst += sprintf(dst, "  * Tk after 0x00 consist of optional consecutive COMPACT-TLV data objects\n");
          dst += sprintf(dst, "    followed by a mandatory status indicator (the last three bytes, not in TLV)\n");
          dst += sprintf(dst, "    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
        }
        if (CIB == 0x10) {
          dst += sprintf(dst, "  * DIR data reference: %02x\n", nai.abtAts[offset]);
        }
        if (CIB == 0x80) {
          if (nai.szAtsLen == offset) {
            dst += sprintf(dst, "  * No COMPACT-TLV objects found, no status found\n");
          } else {
            dst += sprintf(dst, "  * Tk after 0x80 consist of optional consecutive COMPACT-TLV data objects;\n");
            dst += sprintf(dst, "    the last data object may carry a status indicator of one, two or three bytes.\n");
            dst += sprintf(dst, "    See ISO/IEC 7816-4 8.1.1.3 for more info\n");
          }
        }
      }
    }
  }
  if (verbose) {
    dst += sprintf(dst, "\nFingerprinting based on MIFARE type Identification Procedure:\n"); // AN10833
    uint16_t atqa = 0;
    uint8_t sak = 0;
    uint8_t i, j;
    bool found_possible_match = false;

    atqa = (((uint16_t)nai.abtAtqa[0] & 0xff) << 8);
    atqa += (((uint16_t)nai.abtAtqa[1] & 0xff));
    sak = ((uint8_t)nai.btSak & 0xff);

    for (i = 0; i < sizeof(const_ca) / sizeof(const_ca[0]); i++) {
      if ((atqa & const_ca[i].mask) == const_ca[i].atqa) {
        for (j = 0; (j < sizeof(const_ca[i].saklist)) && (const_ca[i].saklist[j] >= 0); j++) {
          int sakindex = const_ca[i].saklist[j];
          if ((sak & const_cs[sakindex].mask) == const_cs[sakindex].sak) {
            dst += sprintf(dst, "* %s%s\n", const_ca[i].type, const_cs[sakindex].type);
            found_possible_match = true;
          }
        }
      }
    }
    // Other matches not described in
    // AN10833 MIFARE Type Identification Procedure
    // but seen in the field:
    dst += sprintf(dst, "Other possible matches based on ATQA & SAK values:\n");
    uint32_t atqasak = 0;
    atqasak += (((uint32_t)nai.abtAtqa[0] & 0xff) << 16);
    atqasak += (((uint32_t)nai.abtAtqa[1] & 0xff) << 8);
    atqasak += ((uint32_t)nai.btSak & 0xff);
    switch (atqasak) {
      case 0x000488:
        dst += sprintf(dst, "* Mifare Classic 1K Infineon\n");
        found_possible_match = true;
        break;
      case 0x000298:
        dst += sprintf(dst, "* Gemplus MPCOS\n");
        found_possible_match = true;
        break;
      case 0x030428:
        dst += sprintf(dst, "* JCOP31\n");
        found_possible_match = true;
        break;
      case 0x004820:
        dst += sprintf(dst, "* JCOP31 v2.4.1\n");
        dst += sprintf(dst, "* JCOP31 v2.2\n");
        found_possible_match = true;
        break;
      case 0x000428:
        dst += sprintf(dst, "* JCOP31 v2.3.1\n");
        found_possible_match = true;
        break;
      case 0x000453:
        dst += sprintf(dst, "* Fudan FM1208SH01\n");
        found_possible_match = true;
        break;
      case 0x000820:
        dst += sprintf(dst, "* Fudan FM1208\n");
        found_possible_match = true;
        break;
      case 0x000238:
        dst += sprintf(dst, "* MFC 4K emulated by Nokia 6212 Classic\n");
        found_possible_match = true;
        break;
      case 0x000838:
        dst += sprintf(dst, "* MFC 4K emulated by Nokia 6131 NFC\n");
        found_possible_match = true;
        break;
    }
    if (! found_possible_match) {
      dst += sprintf(dst, "* Unknown card, sorry\n");
    }
  }
}

void
sprint_nfc_felica_info(char *dst, const nfc_felica_info nfi, bool verbose)
{
  (void) verbose;
  dst += sprintf(dst, "        ID (NFCID2): ");
  dst += sprint_hex(dst, nfi.abtId, 8);
  dst += sprintf(dst, "    Parameter (PAD): ");
  dst += sprint_hex(dst, nfi.abtPad, 8);
  dst += sprintf(dst, "   System Code (SC): ");
  dst += sprint_hex(dst, nfi.abtSysCode, 2);
}

void
sprint_nfc_jewel_info(char *dst, const nfc_jewel_info nji, bool verbose)
{
  (void) verbose;
  dst += sprintf(dst, "    ATQA (SENS_RES): ");
  dst += sprint_hex(dst, nji.btSensRes, 2);
  dst += sprintf(dst, "      4-LSB JEWELID: ");
  dst += sprint_hex(dst, nji.btId, 4);
}

#define PI_ISO14443_4_SUPPORTED 0x01
#define PI_NAD_SUPPORTED        0x01
#define PI_CID_SUPPORTED        0x02
void
sprint_nfc_iso14443b_info(char *dst, const nfc_iso14443b_info nbi, bool verbose)
{
  const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
  dst += sprintf(dst, "               PUPI: ");
  dst += sprint_hex(dst, nbi.abtPupi, 4);
  dst += sprintf(dst, "   Application Data: ");
  dst += sprint_hex(dst, nbi.abtApplicationData, 4);
  dst += sprintf(dst, "      Protocol Info: ");
  dst += sprint_hex(dst, nbi.abtProtocolInfo, 3);
  if (verbose) {
    dst += sprintf(dst, "* Bit Rate Capability:\n");
    if (nbi.abtProtocolInfo[0] == 0) {
      dst += sprintf(dst, " * PICC supports only 106 kbits/s in both directions\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 7) {
      dst += sprintf(dst, " * Same bitrate in both directions mandatory\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 4) {
      dst += sprintf(dst, " * PICC to PCD, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 5) {
      dst += sprintf(dst, " * PICC to PCD, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 6) {
      dst += sprintf(dst, " * PICC to PCD, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 0) {
      dst += sprintf(dst, " * PCD to PICC, 1etu=64/fc, bitrate 212 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 1) {
      dst += sprintf(dst, " * PCD to PICC, 1etu=32/fc, bitrate 424 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 2) {
      dst += sprintf(dst, " * PCD to PICC, 1etu=16/fc, bitrate 847 kbits/s supported\n");
    }
    if (nbi.abtProtocolInfo[0] & 1 << 3) {
      dst += sprintf(dst, " * ERROR unknown value\n");
    }
    if ((nbi.abtProtocolInfo[1] & 0xf0) <= 0x80) {
      dst += sprintf(dst, "* Maximum frame sizes: %d bytes\n", iMaxFrameSizes[((nbi.abtProtocolInfo[1] & 0xf0) >> 4)]);
    }
    if ((nbi.abtProtocolInfo[1] & 0x0f) == PI_ISO14443_4_SUPPORTED) {
      dst += sprintf(dst, "* Protocol types supported: ISO/IEC 14443-4\n");
    }
    dst += sprintf(dst, "* Frame Waiting Time: %.4g ms\n", 256.0 * 16.0 * (1 << ((nbi.abtProtocolInfo[2] & 0xf0) >> 4)) / 13560.0);
    if ((nbi.abtProtocolInfo[2] & (PI_NAD_SUPPORTED | PI_CID_SUPPORTED)) != 0) {
      dst += sprintf(dst, "* Frame options supported: ");
      if ((nbi.abtProtocolInfo[2] & PI_NAD_SUPPORTED) != 0) dst += sprintf(dst, "NAD ");
      if ((nbi.abtProtocolInfo[2] & PI_CID_SUPPORTED) != 0) dst += sprintf(dst, "CID ");
      dst += sprintf(dst, "\n");
    }
  }
}

void
sprint_nfc_iso14443bi_info(char *dst, const nfc_iso14443bi_info nii, bool verbose)
{
  dst += sprintf(dst, "                DIV: ");
  dst += sprint_hex(dst, nii.abtDIV, 4);
  if (verbose) {
    int version = (nii.btVerLog & 0x1e) >> 1;
    dst += sprintf(dst, "   Software Version: ");
    if (version == 15) {
      dst += sprintf(dst, "Undefined\n");
    } else {
      dst += sprintf(dst, "%i\n", version);
    }

    if ((nii.btVerLog & 0x80) && (nii.btConfig & 0x80)) {
      dst += sprintf(dst, "        Wait Enable: yes");
    }
  }
  if ((nii.btVerLog & 0x80) && (nii.btConfig & 0x40)) {
    dst += sprintf(dst, "                ATS: ");
    dst += sprint_hex(dst, nii.abtAtr, nii.szAtrLen);
  }
}

void
sprint_nfc_iso14443b2sr_info(char *dst, const nfc_iso14443b2sr_info nsi, bool verbose)
{
  (void) verbose;
  dst += sprintf(dst, "                UID: ");
  dst += sprint_hex(dst, nsi.abtUID, 8);
}

void
sprint_nfc_iso14443b2ct_info(char *dst, const nfc_iso14443b2ct_info nci, bool verbose)
{
  (void) verbose;
  uint32_t uid;
  uid = (nci.abtUID[3] << 24) + (nci.abtUID[2] << 16) + (nci.abtUID[1] << 8) + nci.abtUID[0];
  dst += sprintf(dst, "                UID: ");
  dst += sprint_hex(dst, nci.abtUID, sizeof(nci.abtUID));
  dst += sprintf(dst, "      UID (decimal): %010u\n", uid);
  dst += sprintf(dst, "       Product Code: %02X\n", nci.btProdCode);
  dst += sprintf(dst, "           Fab Code: %02X\n", nci.btFabCode);
}

void
sprint_nfc_dep_info(char *dst, const nfc_dep_info ndi, bool verbose)
{
  (void) verbose;
  dst += sprintf(dst, "       NFCID3: ");
  dst += sprint_hex(dst, ndi.abtNFCID3, 10);
  dst += sprintf(dst, "           BS: %02x\n", ndi.btBS);
  dst += sprintf(dst, "           BR: %02x\n", ndi.btBR);
  dst += sprintf(dst, "           TO: %02x\n", ndi.btTO);
  dst += sprintf(dst, "           PP: %02x\n", ndi.btPP);
  if (ndi.szGB) {
    dst += sprintf(dst, "General Bytes: ");
    dst += sprint_hex(dst, ndi.abtGB, ndi.szGB);
  }
}

void
sprint_nfc_target(char *dst, const nfc_target nt, bool verbose)
{
  dst += sprintf(dst, "%s (%s%s) target:\n", str_nfc_modulation_type(nt.nm.nmt), str_nfc_baud_rate(nt.nm.nbr), (nt.nm.nmt != NMT_DEP) ? "" : (nt.nti.ndi.ndm == NDM_ACTIVE) ? "active mode" : "passive mode");
  switch (nt.nm.nmt) {
    case NMT_ISO14443A:
      sprint_nfc_iso14443a_info(dst, nt.nti.nai, verbose);
      break;
    case NMT_JEWEL:
      sprint_nfc_jewel_info(dst, nt.nti.nji, verbose);
      break;
    case NMT_FELICA:
      sprint_nfc_felica_info(dst, nt.nti.nfi, verbose);
      break;
    case NMT_ISO14443B:
      sprint_nfc_iso14443b_info(dst, nt.nti.nbi, verbose);
      break;
    case NMT_ISO14443BI:
      sprint_nfc_iso14443bi_info(dst, nt.nti.nii, verbose);
      break;
    case NMT_ISO14443B2SR:
      sprint_nfc_iso14443b2sr_info(dst, nt.nti.nsi, verbose);
      break;
    case NMT_ISO14443B2CT:
      sprint_nfc_iso14443b2ct_info(dst, nt.nti.nci, verbose);
      break;
    case NMT_DEP:
      sprint_nfc_dep_info(dst, nt.nti.ndi, verbose);
      break;
  }
}

