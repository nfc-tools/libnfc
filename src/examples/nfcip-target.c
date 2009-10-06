#include <stdio.h>
#include "libnfc.h"

int main(int argc, const char *argv[])
{
  byte_t abtRecv[MAX_FRAME_LEN];
  size_t szRecvBits;
  byte_t send[] = "Hello Mars!";
  dev_info *pdi = nfc_connect(NULL);

  if (!pdi || !nfc_target_init(pdi, abtRecv, &szRecvBits)) {
    printf("unable to connect or initialize\n");
    return 1;
  }

  if (!nfc_target_receive_dep_bytes(pdi, abtRecv, &szRecvBits)) {
    printf("unable to receive data\n");
    return 1;
  }
  abtRecv[szRecvBits] = 0;
  printf("Received: %s\n", abtRecv);
  printf("Sending : %s\n", send);

  if (!nfc_target_send_dep_bytes(pdi, send, 11)) {
    printf("unable to send data\n");
    return 1;
  }

  nfc_disconnect(pdi);
  return 0;
}
