#include <nfc/nfc.h>

#include "bitutils.h"
#include "nfc-utils.h"

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
