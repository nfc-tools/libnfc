#include <cutter.h>

#include <nfc/nfc.h>

#define MAX_DEVICE_COUNT 1
#define MAX_TARGET_COUNT 1

#include "chips/pn53x.h"

void
test_register_endianness (void)
{
    nfc_connstring connstrings[MAX_DEVICE_COUNT];
    int res = 0;

    size_t device_count = nfc_list_devices (connstrings, MAX_DEVICE_COUNT);
    if (!device_count)
	cut_omit ("No NFC device found");

    nfc_device *device;

    device = nfc_connect (connstrings[0]);
    cut_assert_not_null (device, cut_message ("nfc_connect"));

    uint8_t value;

    /* Read valid XRAM memory */
    res = pn53x_read_register (device, 0xF0FF, &value);
    cut_assert_equal_int (0, res, cut_message ("read register 0xF0FF"));

    /* Read invalid SFR register */
    res = pn53x_read_register (device, 0xFFF0, &value);
    cut_assert_equal_int (0, res, cut_message ("read register 0xFFF0"));

    nfc_close (device);
}
