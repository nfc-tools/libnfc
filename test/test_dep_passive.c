#include <cutter.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "nfc/nfc.h"

void test_dep_passive(void);

#define INITIATOR 0
#define TARGET    1

pthread_t threads[2];
nfc_context *context;
nfc_connstring connstrings[2];
nfc_device *devices[2];
intptr_t result[2];

static void
abort_test_by_keypress(int sig)
{
  (void) sig;
  printf("\033[0;1;31mSIGINT\033[0m");

  nfc_abort_command(devices[INITIATOR]);
  nfc_abort_command(devices[TARGET]);
}

void
cut_setup(void)
{
  nfc_init(&context);
  size_t n = nfc_list_devices(context, connstrings, 2);
  if (n < 2) {
    cut_omit("At least two NFC devices must be plugged-in to run this test");
  }

  devices[TARGET] = nfc_open(context, connstrings[TARGET]);
  devices[INITIATOR] = nfc_open(context, connstrings[INITIATOR]);

  signal(SIGINT, abort_test_by_keypress);
}

void
cut_teardown(void)
{
  nfc_close(devices[TARGET]);
  nfc_close(devices[INITIATOR]);
  nfc_exit(context);
}

struct thread_data {
  nfc_device *device;
  void *cut_test_context;
};

static void *
target_thread(void *arg)
{
  intptr_t thread_res = 0;
  nfc_device *device = ((struct thread_data *) arg)->device;
  cut_set_current_test_context(((struct thread_data *) arg)->cut_test_context);

  printf("=========== TARGET %s =========\n", nfc_device_get_name(device));
  nfc_target nt = {
    .nm = {
      .nmt = NMT_DEP,
      .nbr = NBR_UNDEFINED
    },
    .nti = {
      .ndi = {
        .abtNFCID3 = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA },
        .szGB = 4,
        .abtGB = { 0x12, 0x34, 0x56, 0x78 },
        .ndm = NDM_PASSIVE,
        /* These bytes are not used by nfc_target_init: the chip will provide them automatically to the initiator */
        .btDID = 0x00,
        .btBS = 0x00,
        .btBR = 0x00,
        .btTO = 0x00,
        .btPP = 0x01,
      },
    },
  };

  uint8_t abtRx[1024];
  size_t szRx = sizeof(abtRx);
  int res = nfc_target_init(device, &nt, abtRx, szRx, 0);
  cut_assert_operator_int(res, > , 0, cut_message("Can't initialize NFC device as target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // First pass
  res =  nfc_target_receive_bytes(device, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't receive bytes from initiator: %s", nfc_strerror(device)));

  const uint8_t abtAttRx[] = "Hello DEP target!";
  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  const uint8_t abtTx[] = "Hello DEP initiator!";
  res = nfc_target_send_bytes(device, abtTx, sizeof(abtTx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't send bytes to initiator: %s", nfc_strerror(device)));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  // Second pass
  res = nfc_target_receive_bytes(device, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't receive bytes from initiator: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_target_send_bytes(device, abtTx, sizeof(abtTx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't send bytes to initiator: %s", nfc_strerror(device)));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  // Third pass
  res = nfc_target_receive_bytes(device, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't receive bytes from initiator: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_target_send_bytes(device, abtTx, sizeof(abtTx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't send bytes to initiator: %s", nfc_strerror(device)));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  // Fourth pass
  res =  nfc_target_receive_bytes(device, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't receive bytes from initiator: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_target_send_bytes(device, abtTx, sizeof(abtTx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't send bytes to initiator: %s", nfc_strerror(device)));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  return (void *) thread_res;
}

static void *
initiator_thread(void *arg)
{
  intptr_t thread_res = 0;
  nfc_device *device = ((struct thread_data *) arg)->device;
  cut_set_current_test_context(((struct thread_data *) arg)->cut_test_context);

  /*
   * Wait some time for the other thread to initialise NFC device as target
   */
  sleep(1);
  printf("=========== INITIATOR %s =========\n", nfc_device_get_name(device));

  int res = nfc_initiator_init(device);
  cut_assert_equal_int(0, res, cut_message("Can't initialize NFC device as initiator: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  nfc_target nt;

  // Passive mode / 106Kbps
  printf("=========== INITIATOR %s (Passive mode / 106Kbps) =========\n", nfc_device_get_name(device));
  res = nfc_initiator_select_dep_target(device, NDM_PASSIVE, NBR_106, NULL, &nt, 5000);
  cut_assert_operator_int(res, > , 0, cut_message("Can't select any DEP target: %s", nfc_strerror(device)));
  cut_assert_equal_int(NMT_DEP, nt.nm.nmt, cut_message("Invalid target modulation"));
  cut_assert_equal_int(NBR_106, nt.nm.nbr, cut_message("Invalid target baud rate"));
  cut_assert_equal_memory("\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA", 10, nt.nti.ndi.abtNFCID3, 10, cut_message("Invalid target NFCID3"));
  cut_assert_equal_int(NDM_PASSIVE, nt.nti.ndi.ndm, cut_message("Invalid target DEP mode"));
  cut_assert_equal_memory("\x12\x34\x56\x78", 4, nt.nti.ndi.abtGB, nt.nti.ndi.szGB, cut_message("Invalid target general bytes"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  const uint8_t abtTx[] = "Hello DEP target!";
  uint8_t abtRx[1024];
  res = nfc_initiator_transceive_bytes(device, abtTx, sizeof(abtTx), abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't transceive bytes to target: %s", nfc_strerror(device)));

  const uint8_t abtAttRx[] = "Hello DEP initiator!";
  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_deselect_target(device);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't deselect target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // Passive mode / 212Kbps (second pass)
  printf("=========== INITIATOR %s (Passive mode / 212Kbps) =========\n", nfc_device_get_name(device));
  res = nfc_initiator_select_dep_target(device, NDM_PASSIVE, NBR_212, NULL, &nt, 1000);
  cut_assert_operator_int(res, > , 0, cut_message("Can't select any DEP target: %s", nfc_strerror(device)));
  cut_assert_equal_int(NMT_DEP, nt.nm.nmt, cut_message("Invalid target modulation"));
  cut_assert_equal_int(NBR_212, nt.nm.nbr, cut_message("Invalid target baud rate"));
  cut_assert_equal_memory("\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA", 10, nt.nti.ndi.abtNFCID3, 10, cut_message("Invalid target NFCID3"));
  cut_assert_equal_int(NDM_PASSIVE, nt.nti.ndi.ndm, cut_message("Invalid target DEP mode"));
  cut_assert_equal_memory("\x12\x34\x56\x78", 4, nt.nti.ndi.abtGB, nt.nti.ndi.szGB, cut_message("Invalid target general bytes"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_transceive_bytes(device, abtTx, sizeof(abtTx), abtRx, sizeof(abtRx), 1000);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't transceive bytes to target: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_deselect_target(device);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't deselect target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // Passive mode / 212Kbps
  printf("=========== INITIATOR %s (Passive mode / 212Kbps, second pass) =========\n", nfc_device_get_name(device));
  res = nfc_initiator_select_dep_target(device, NDM_PASSIVE, NBR_212, NULL, &nt, 1000);
  cut_assert_operator_int(res, > , 0, cut_message("Can't select any DEP target: %s", nfc_strerror(device)));
  cut_assert_equal_int(NMT_DEP, nt.nm.nmt, cut_message("Invalid target modulation"));
  cut_assert_equal_int(NBR_212, nt.nm.nbr, cut_message("Invalid target baud rate"));
  cut_assert_equal_memory("\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA", 10, nt.nti.ndi.abtNFCID3, 10, cut_message("Invalid target NFCID3"));
  cut_assert_equal_int(NDM_PASSIVE, nt.nti.ndi.ndm, cut_message("Invalid target DEP mode"));
  cut_assert_equal_memory("\x12\x34\x56\x78", 4, nt.nti.ndi.abtGB, nt.nti.ndi.szGB, cut_message("Invalid target general bytes"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_transceive_bytes(device, abtTx, sizeof(abtTx), abtRx, sizeof(abtRx), 5000);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't transceive bytes to target: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_deselect_target(device);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't deselect target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // Passive mode / 424Kbps
  printf("=========== INITIATOR %s (Passive mode / 424Kbps) =========\n", nfc_device_get_name(device));
  res = nfc_initiator_select_dep_target(device, NDM_PASSIVE, NBR_424, NULL, &nt, 1000);
  cut_assert_operator_int(res, > , 0, cut_message("Can't select any DEP target: %s", nfc_strerror(device)));
  cut_assert_equal_int(NMT_DEP, nt.nm.nmt, cut_message("Invalid target modulation"));
  cut_assert_equal_int(NBR_424, nt.nm.nbr, cut_message("Invalid target baud rate"));
  cut_assert_equal_memory("\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA", 10, nt.nti.ndi.abtNFCID3, 10, cut_message("Invalid target NFCID3"));
  cut_assert_equal_int(NDM_PASSIVE, nt.nti.ndi.ndm, cut_message("Invalid target DEP mode"));
  cut_assert_equal_memory("\x12\x34\x56\x78", 4, nt.nti.ndi.abtGB, nt.nti.ndi.szGB, cut_message("Invalid target general bytes"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_transceive_bytes(device, abtTx, sizeof(abtTx), abtRx, sizeof(abtRx), 5000);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't transceive bytes to target: %s", nfc_strerror(device)));

  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  res = nfc_initiator_deselect_target(device);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't deselect target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  return (void *) thread_res;
}

void
test_dep_passive(void)
{
  int res;

  CutTestContext *test_context = cut_get_current_test_context();
  struct thread_data target_data = {
    .device = devices[TARGET],
    .cut_test_context = test_context,
  };
  if ((res = pthread_create(&(threads[TARGET]), NULL, target_thread, &target_data)))
    cut_fail("pthread_create() returned %d", res);

  struct thread_data initiator_data = {
    .device = devices[INITIATOR],
    .cut_test_context = test_context,
  };
  if ((res = pthread_create(&(threads[INITIATOR]), NULL, initiator_thread, &initiator_data)))
    cut_fail("pthread_create() returned %d", res);

  if ((res = pthread_join(threads[INITIATOR], (void *) &result[INITIATOR])))
    cut_fail("pthread_join() returned %d", res);
  if ((res = pthread_join(threads[TARGET], (void *) &result[TARGET])))
    cut_fail("pthread_join() returned %d", res);

  cut_assert_equal_int(0, result[INITIATOR], cut_message("Unexpected initiator return code"));
  cut_assert_equal_int(0, result[TARGET], cut_message("Unexpected target return code"));
}
