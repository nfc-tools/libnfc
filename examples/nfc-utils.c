#include <nfc/nfc.h>

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

void print_hex(const byte_t* pbtData, const size_t szBytes)
{
  size_t szPos;

  for (szPos=0; szPos < szBytes; szPos++)
  {
    printf("%02x  ",pbtData[szPos]);
  }
  printf("\n");
}

void print_hex_bits(const byte_t* pbtData, const size_t szBits)
{
  uint8_t uRemainder;
  size_t szPos;
  size_t szBytes = szBits/8;

  for (szPos=0; szPos < szBytes; szPos++)
  {
    printf("%02x  ",pbtData[szPos]);
  }

  uRemainder = szBits % 8;
  // Print the rest bits
  if (uRemainder != 0)
  {
    if (uRemainder < 5)
      printf("%01x (%d bits)",pbtData[szBytes], uRemainder);
    else
      printf("%02x (%d bits)",pbtData[szBytes], uRemainder);
  }
  printf("\n");
}

void print_hex_par(const byte_t* pbtData, const size_t szBits, const byte_t* pbtDataPar)
{
  uint8_t uRemainder;
  size_t szPos;
  size_t szBytes = szBits/8;

  for (szPos=0; szPos < szBytes; szPos++)
  {
    printf("%02x",pbtData[szPos]);
    if (OddParity[pbtData[szPos]] != pbtDataPar[szPos])
    {
      printf("! ");
    } else {
      printf("  ");
    }
  }

  uRemainder = szBits % 8;
  // Print the rest bits, these cannot have parity bit
  if (uRemainder != 0)
  {
    if (uRemainder < 5)
      printf("%01x (%d bits)",pbtData[szBytes], uRemainder);
    else
      printf("%02x (%d bits)",pbtData[szBytes], uRemainder);
  }
  printf("\n");
}

void print_nfc_iso14443a_info(const nfc_iso14443a_info_t nai)
{
  printf("    ATQA (SENS_RES): "); print_hex(nai.abtAtqa,2);
  printf("       UID (NFCID%c): ",(nai.abtUid[0]==0x08?'3':'1')); print_hex(nai.abtUid, nai.szUidLen);
  printf("      SAK (SEL_RES): "); print_hex(&nai.btSak,1);
  if (nai.szAtsLen) {
    printf("          ATS (ATR): ");
    print_hex(nai.abtAts, nai.szAtsLen);
  }
}

