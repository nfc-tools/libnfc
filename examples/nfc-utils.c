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

/**
 * @brief Tries to parse arguments to find device descriptions.
 * @return Returns the list of found device descriptions.
 */
nfc_device_desc_t* parse_device_desc(int argc, const char *argv[], size_t* szFound)
{
  nfc_device_desc_t* pndd = 0;
  *szFound = 0;
  int arg;

  // Get commandline options
  for (arg=1;arg<argc;arg++) {

    if (0 == strcmp(argv[arg], "--device")) {

      if (argc > arg+1) {

        pndd = malloc(sizeof(nfc_device_desc_t));

        char buffer[256];
        strncpy(buffer, argv[++arg], 256);

        // Driver.
        pndd->pcDriver = (char *)malloc(256);
        strcpy(pndd->pcDriver, strtok(buffer, ":"));

        // Port.
        pndd->pcPort = (char *)malloc(256);
        strcpy(pndd->pcPort, strtok(NULL, ":"));

        // Speed.
        sscanf(strtok(NULL, ":"), "%u", &pndd->uiSpeed);

        *szFound = 1;
      }
      break;
    }
  }

  return pndd;
}

