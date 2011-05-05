#include <cutter.h>

#include <nfc/nfc.h>

#define NTESTS 10
#define MAX_DEVICE_COUNT 8
#define MAX_TARGET_COUNT 8

/*
 * This is basically a stress-test to ensure we don't left a device in an
 * inconsistent state after use.
 */
void
test_access_storm (void)
{
    int n = NTESTS;
    nfc_device_desc_t devices[MAX_DEVICE_COUNT];
    size_t device_count, ref_device_count, target_count;
    bool res;

    nfc_list_devices (devices, MAX_DEVICE_COUNT, &ref_device_count);
    if (!ref_device_count)
	cut_omit ("No NFC device found");

    while (n) {
	size_t i;

	nfc_list_devices (devices, MAX_DEVICE_COUNT, &device_count);
	cut_assert_equal_int (ref_device_count, device_count, cut_message ("device count"));

	for (i = 0; i < device_count; i++) {
	    nfc_device_t *device;
	    nfc_target_t ant[MAX_TARGET_COUNT];

	    device = nfc_connect (&(devices[i]));
	    cut_assert_not_null (device, cut_message ("nfc_connect"));

	    res = nfc_initiator_init(device);
	    cut_assert_true (res, cut_message ("nfc_initiator_init"));

	    const nfc_modulation_t nm = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	    };
	    res = nfc_initiator_list_passive_targets(device, nm, ant, MAX_TARGET_COUNT, &target_count);
	    cut_assert_true (res, cut_message ("nfc_initiator_list_passive_targets"));

	    nfc_disconnect (device);
	}

	n--;
    }
}
