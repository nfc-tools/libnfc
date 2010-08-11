#include <cutter.h>

#include <nfc/nfc.h>

#define NTESTS 42
#define MAX_TARGET_COUNT 8

void
test_access (void)
{
    int n = NTESTS;
    nfc_device_desc_t devices[8];
    size_t device_count, ref_device_count, target_count;
    bool res;

    nfc_list_devices (devices, 8, &ref_device_count);
    if (!ref_device_count)
	cut_omit ("No NFC device found");

    while (n) {
	size_t i;

	nfc_list_devices (devices, 8, &device_count);
	cut_assert_equal_int (ref_device_count, device_count, cut_message ("device count"));

	for (i = 0; i < device_count; i++) {
	    nfc_device_t *device;
	    nfc_target_info_t anti[MAX_TARGET_COUNT];

	    device = nfc_connect (&(devices[i]));
	    cut_assert_not_null (device, cut_message ("nfc_connect"));

	    nfc_initiator_init(device);

	    // Drop the field for a while
	    nfc_configure(device,NDO_ACTIVATE_FIELD,false);

	    // Let the reader only try once to find a tag
	    nfc_configure(device,NDO_INFINITE_SELECT,false);

	    // Configure the CRC and Parity settings
	    nfc_configure(device,NDO_HANDLE_CRC,true);
	    nfc_configure(device,NDO_HANDLE_PARITY,true);

	    // Enable field so more power consuming cards can power themselves
	    nfc_configure(device,NDO_ACTIVATE_FIELD,true);

	    res = nfc_initiator_list_passive_targets(device, NM_ISO14443A_106, anti, MAX_TARGET_COUNT, &target_count);
	    cut_assert_true (res, cut_message ("nfc_initiator_list_passive_targets"));

	    nfc_disconnect (device);
	}

	n--;
    }
}
