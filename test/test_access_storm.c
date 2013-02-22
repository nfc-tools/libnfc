#include <cutter.h>

#include <nfc/nfc.h>

#define NTESTS 10
#define MAX_DEVICE_COUNT 8
#define MAX_TARGET_COUNT 8

/*
 * This is basically a stress-test to ensure we don't left a device in an
 * inconsistent state after use.
 */
void test_access_storm(void);

void
test_access_storm(void)
{
  int n = NTESTS;
  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  int res = 0;

  nfc_context *context;
  nfc_init(&context);

  size_t ref_device_count = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
  if (!ref_device_count)
    cut_omit("No NFC device found");

  while (n) {
    size_t device_count = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
    cut_assert_equal_int(ref_device_count, device_count, cut_message("device count"));

    for (volatile size_t i = 0; i < device_count; i++) {
      nfc_device *device;
      nfc_target ant[MAX_TARGET_COUNT];

      device = nfc_open(context, connstrings[i]);
      cut_assert_not_null(device, cut_message("nfc_open"));

      res = nfc_initiator_init(device);
      cut_assert_equal_int(0, res, cut_message("nfc_initiator_init"));

      const nfc_modulation nm = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
      };
      res = nfc_initiator_list_passive_targets(device, nm, ant, MAX_TARGET_COUNT);
      cut_assert_operator_int(res, >= , 0, cut_message("nfc_initiator_list_passive_targets"));

      nfc_close(device);
    }

    n--;
  }
  nfc_exit(context);
}
