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
print_nfc_jewel_info (const nfc_jewel_info_t nji)
{
  printf ("      4-LSB JEWELID: ");
  print_hex (nji.btId, 4);
  printf ("           SENS_RES: ");
  print_hex (nji.btSensRes, 2);
}

#define PI_ISO14443_4_SUPPORTED 0x01
#define PI_NAD_SUPPORTED        0x01
#define PI_CID_SUPPORTED        0x02
void
print_nfc_iso14443b_info (const nfc_iso14443b_info_t nbi)
{
  const int iMaxFrameSizes[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };
  printf ("               PUPI: ");
  print_hex (nbi.abtPupi, 4);
  printf ("   Application Data: ");
  print_hex (nbi.abtApplicationData, 4);
  printf ("      Protocol Info: ");
  print_hex (nbi.abtProtocolInfo, 3);
  printf ("Bit Rate Capability:\n");
  if (nbi.abtProtocolInfo[0] == 0) {
    printf ("* PICC supports only 106 kbits/s in both directions\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<7) {
    printf ("* Same bitrate in both directions mandatory\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<4) {
    printf ("* PICC to PCD, 1etu=64/fc, bitrate 212 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<5) {
    printf ("* PICC to PCD, 1etu=32/fc, bitrate 424 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<6) {
    printf ("* PICC to PCD, 1etu=16/fc, bitrate 847 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<0) {
    printf ("* PCD to PICC, 1etu=64/fc, bitrate 212 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<1) {
    printf ("* PCD to PICC, 1etu=32/fc, bitrate 424 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<2) {
    printf ("* PCD to PICC, 1etu=16/fc, bitrate 847 kbits/s supported\n");
  }
  if (nbi.abtProtocolInfo[0] & 1<<3) {
    printf ("* ERROR unknown value\n");
  }
  if( (nbi.abtProtocolInfo[1] & 0xf0) <= 0x80 ) {
    printf ("Maximum frame sizes: %d bytes\n", iMaxFrameSizes[((nbi.abtProtocolInfo[1] & 0xf0) >> 4)]);
  }
  if((nbi.abtProtocolInfo[1] & 0x0f) == PI_ISO14443_4_SUPPORTED) {
    printf ("Protocol types supported: ISO/IEC 14443-4\n");
  }
  printf ("Frame Waiting Time: %.4g ms\n",256.0*16.0*(1<<((nbi.abtProtocolInfo[2] & 0xf0) >> 4))/13560.0);
  if((nbi.abtProtocolInfo[2] & (PI_NAD_SUPPORTED|PI_CID_SUPPORTED)) != 0) {
    printf ("Frame options supported: ");
    if ((nbi.abtProtocolInfo[2] & PI_NAD_SUPPORTED) != 0) printf ("NAD ");
    if ((nbi.abtProtocolInfo[2] & PI_CID_SUPPORTED) != 0) printf ("CID ");
    printf("\n");
  }
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
