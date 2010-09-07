#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdlib.h>
#include <nfc/nfc.h>
#include <nfc/nfc-messages.h>

int
main (int argc, const char *argv[])
{
  nfc_device_t *pnd;
  nfc_target_info_t nti;

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version ();
  printf ("%s use libnfc %s\n", argv[0], acLibnfcVersion);

  // Connect using the first available NFC device
  pnd = nfc_connect (NULL);

  if (pnd == NULL) {
    ERR ("%s", "Unable to connect to NFC device.");
    return EXIT_FAILURE;
  }
  // Set connected NFC device to initiator mode
  nfc_initiator_init (pnd);

  // Drop the field for a while
  nfc_configure (pnd, NDO_ACTIVATE_FIELD, false);

  // Let the reader only try once to find a tag
  nfc_configure (pnd, NDO_INFINITE_SELECT, false);

  // Configure the CRC and Parity settings
  nfc_configure (pnd, NDO_HANDLE_CRC, true);
  nfc_configure (pnd, NDO_HANDLE_PARITY, true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure (pnd, NDO_ACTIVATE_FIELD, true);

  printf ("Connected to NFC reader: %s\n", pnd->acName);

  // Poll for a ISO14443A (MIFARE) tag
  if (nfc_initiator_select_passive_target (pnd, NM_ISO14443A_106, NULL, 0, &nti)) {
    printf ("The following (NFC) ISO14443A tag was found:\n");
    printf ("    ATQA (SENS_RES): ");
    print_hex (nti.nai.abtAtqa, 2);
    printf ("       UID (NFCID%c): ", (nti.nai.abtUid[0] == 0x08 ? '3' : '1'));
    print_hex (nti.nai.abtUid, nti.nai.szUidLen);
    printf ("      SAK (SEL_RES): ");
    print_hex (&nti.nai.btSak, 1);
    if (nti.nai.szAtsLen) {
      printf ("          ATS (ATR): ");
      print_hex (nti.nai.abtAts, nti.nai.szAtsLen);
    }
  }
  // Disconnect from NFC device
  nfc_disconnect (pnd);
  return EXIT_SUCCESS;
}
