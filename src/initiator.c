#include <stdio.h>
#include <string.h>
#include "libnfc.h"

int main(int argc, const char *argv[])
{
  dev_info *pdi;
  tag_info ti;
  byte_t abtRecv[MAX_FRAME_LEN];
  uint32_t uiRecvBits;
  byte_t send[] = "Hello World!";

  pdi = nfc_connect();
  if (!pdi || !nfc_initiator_init(pdi)
      || !nfc_initiator_select_dep_target(pdi, IM_PASSIVE_DEP, NULL, 0,
					  NULL, 0, NULL, 0, &ti)) {
    printf
	("unable to connect, initialize, or select the target\n");
    return 1;
  }

  printf("Sending : %s\n", send);
  if (!nfc_initiator_transceive_dep_bytes(pdi,
					  send,
					  strlen(send), abtRecv,
					  &uiRecvBits)) {
    printf("unable to send data\n");
    return 1;
  }

  abtRecv[uiRecvBits] = 0;
  printf("Received: %s\n", abtRecv);

  nfc_initiator_deselect_tag(pdi);
  nfc_disconnect(pdi);
  return 0;
}
