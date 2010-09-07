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

#define SAK_ISO14443_4_COMPLIANT 0x20
#define SAK_ISO18092_COMPLIANT   0x40

void
print_nfc_iso14443a_info (const nfc_iso14443a_info_t nai)
{
  printf ("    ATQA (SENS_RES): ");
  print_hex (nai.abtAtqa, 2);
  printf ("       UID (NFCID%c): ", (nai.abtUid[0] == 0x08 ? '3' : '1'));
  print_hex (nai.abtUid, nai.szUidLen);
  printf ("      SAK (SEL_RES): ");
  print_hex (&nai.btSak, 1);
  if (nai.szAtsLen) {
    printf ("          ATS (ATR): ");
    print_hex (nai.abtAts, nai.szAtsLen);
  }
  if ((nai.btSak & SAK_ISO14443_4_COMPLIANT) || (nai.btSak & SAK_ISO18092_COMPLIANT)) {
    printf ("     Compliant with: ");
    if (nai.btSak & SAK_ISO14443_4_COMPLIANT)
      printf ("ISO/IEC 14443-4 ");
    if (nai.btSak & SAK_ISO18092_COMPLIANT)
      printf ("ISO/IEC 18092");
    printf ("\n");
  }
}

void
print_nfc_felica_info (const nfc_felica_info_t nfi)
{
  printf ("        ID (NFCID2): ");
  print_hex (nfi.abtId, 8);
  printf ("    Parameter (PAD): ");
  print_hex (nfi.abtPad, 8);
}

void
print_nfc_iso14443b_info (const nfc_iso14443b_info_t nbi)
{
  printf ("               ATQB: ");
  print_hex (nbi.abtAtqb, 12);
  printf ("                 ID: ");
  print_hex (nbi.abtId, 4);
  printf ("                CID: %02x\n", nbi.btCid);
  if (nbi.szInfLen > 0) {
    printf ("                INF: ");
    print_hex (nbi.abtInf, nbi.szInfLen);
  }
  printf ("             PARAMS: %02x %02x %02x %02x\n", nbi.btParam1, nbi.btParam2, nbi.btParam3, nbi.btParam4);
}

/**
 * @brief Tries to parse arguments to find device descriptions.
 * @return Returns the list of found device descriptions.
 */
nfc_device_desc_t *
parse_device_desc (int argc, const char *argv[], size_t * szFound)
{
  nfc_device_desc_t *pndd = 0;
  int     arg;
  *szFound = 0;

  // Get commandline options
  for (arg = 1; arg < argc; arg++) {

    if (0 == strcmp (argv[arg], "--device")) {

      if (argc > arg + 1) {
        char    buffer[256];

        pndd = malloc (sizeof (nfc_device_desc_t));

        strncpy (buffer, argv[++arg], 256);

        // Driver.
        pndd->pcDriver = (char *) malloc (256);
        strcpy (pndd->pcDriver, strtok (buffer, ":"));

        // Port.
        pndd->pcPort = (char *) malloc (256);
        strcpy (pndd->pcPort, strtok (NULL, ":"));

        // Speed.
        sscanf (strtok (NULL, ":"), "%u", &pndd->uiSpeed);

        *szFound = 1;
      }
      break;
    }
  }

  return pndd;
}
