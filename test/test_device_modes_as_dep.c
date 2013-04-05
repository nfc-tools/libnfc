#include <cutter.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "nfc/nfc.h"
#include "../utils/nfc-utils.h"

void test_dep_states(void);

pthread_t threads[2];
nfc_context *context;
nfc_connstring connstrings[2];
nfc_device *first_device, *second_device;
intptr_t result[2];

static void
abort_test_by_keypress(int sig)
{
  (void) sig;
  printf("\033[0;1;31mSIGINT\033[0m");

  nfc_abort_command(first_device);
  nfc_abort_command(second_device);
}

void
cut_setup(void)
{
  nfc_init(&context);
  size_t n = nfc_list_devices(context, connstrings, 2);
  if (n < 2) {
    cut_omit("At least two NFC devices must be plugged-in to run this test");
  }

  second_device = nfc_open(context, connstrings[0]);
  first_device = nfc_open(context, connstrings[1]);

  signal(SIGINT, abort_test_by_keypress);
}

void
cut_teardown(void)
{
  nfc_close(second_device);
  nfc_close(first_device);
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
  nfc_target nt;

  uint8_t abtRx[1024];

  // 1) nfc_target_init should take target in idle mode
  int res = nfc_target_init(device, &nt, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, >= , 0, cut_message("Can't initialize NFC device as target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // 2) act as target
  nfc_target nt1 = {
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
  sleep(6);
  res = nfc_target_init(device, &nt1, abtRx, sizeof(abtRx), 0);
  cut_assert_operator_int(res, > , 0, cut_message("Can't initialize NFC device as target: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  res =  nfc_target_receive_bytes(device, abtRx, sizeof(abtRx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't receive bytes from initiator: %s", nfc_strerror(device)));

  const uint8_t abtAttRx[] = "Hello DEP target!";
  cut_assert_equal_memory(abtAttRx, sizeof(abtAttRx), abtRx, res, cut_message("Invalid received data"));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  const uint8_t abtTx[] = "Hello DEP initiator!";
  res = nfc_target_send_bytes(device, abtTx, sizeof(abtTx), 500);
  cut_assert_operator_int(res, > , 0, cut_message("Can't send bytes to initiator: %s", nfc_strerror(device)));
  if (res <= 0) { thread_res = -1; return (void *) thread_res; }

  // 3) idle mode
  sleep(1);
  nfc_idle(device);

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
  sleep(5);
  printf("=========== INITIATOR %s =========\n", nfc_device_get_name(device));

  int res = nfc_initiator_init(device);
  cut_assert_equal_int(0, res, cut_message("Can't initialize NFC device as initiator: %s", nfc_strerror(device)));
  if (res < 0) { thread_res = -1; return (void *) thread_res; }

  // 1) As other device should be in idle mode, nfc_initiator_poll_dep_target should return 0
  nfc_target nt;
  res = nfc_initiator_poll_dep_target(device, NDM_PASSIVE, NBR_106, NULL, &nt, 1000);
  cut_assert_equal_int(0, res, cut_message("Problem with nfc_idle"));
  if (res != 0) { thread_res = -1; return (void *) thread_res; }


  // 2 As other device should be in target mode, nfc_initiator_poll_dep_target should be positive.
  nfc_target nt1;

  // Passive mode / 106Kbps
  printf("=========== INITIATOR %s (Passive mode / 106Kbps) =========\n", nfc_device_get_name(device));
  res = nfc_initiator_poll_dep_target(device, NDM_PASSIVE, NBR_106, NULL, &nt1, 5000);
  cut_assert_operator_int(res, > , 0, cut_message("Can't select any DEP target: %s", nfc_strerror(device)));
  cut_assert_equal_int(NMT_DEP, nt1.nm.nmt, cut_message("Invalid target modulation"));
  cut_assert_equal_int(NBR_106, nt1.nm.nbr, cut_message("Invalid target baud rate"));
  cut_assert_equal_memory("\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA", 10, nt1.nti.ndi.abtNFCID3, 10, cut_message("Invalid target NFCID3"));
  cut_assert_equal_int(NDM_PASSIVE, nt1.nti.ndi.ndm, cut_message("Invalid target DEP mode"));
  cut_assert_equal_memory("\x12\x34\x56\x78", 4, nt1.nti.ndi.abtGB, nt1.nti.ndi.szGB, cut_message("Invalid target general bytes"));
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

  // 3) As other device should be in idle mode, nfc_initiator_poll_dep_target should return 0
  nfc_target nt2;
  res = nfc_initiator_poll_dep_target(device, NDM_PASSIVE, NBR_106, NULL, &nt2, 1000);
  cut_assert_equal_int(0, res, cut_message("Problem with nfc_idle"));
  if (res != 0) { thread_res = -1; return (void *) thread_res; }

  return (void *) thread_res;
}

void
test_dep_states(void)
{
  CutTestContext *test_context = cut_get_current_test_context();
  struct thread_data target_data = {
    .device = first_device,
    .cut_test_context = test_context,
  };

  struct thread_data initiator_data = {
    .device = second_device,
    .cut_test_context = test_context,
  };

  for (int i = 0; i < 2; i++) {
    int res;
    if ((res = pthread_create(&(threads[1]), NULL, target_thread, &target_data)))
      cut_fail("pthread_create() returned %d", res);

    if ((res = pthread_create(&(threads[0]), NULL, initiator_thread, &initiator_data)))
      cut_fail("pthread_create() returned %d", res);

    if ((res = pthread_join(threads[0], (void *) &result[0])))
      cut_fail("pthread_join() returned %d", res);
    if ((res = pthread_join(threads[1], (void *) &result[1])))
      cut_fail("pthread_join() returned %d", res);

    cut_assert_equal_int(0, result[0], cut_message("Unexpected initiator return code"));
    cut_assert_equal_int(0, result[1], cut_message("Unexpected target return code"));

    // initiator --> target, target --> initiator
    target_data.device = second_device;
    initiator_data.device = first_device;
  }
}
