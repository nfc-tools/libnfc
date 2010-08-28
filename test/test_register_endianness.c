#include <cutter.h>

#include <nfc/nfc.h>

#define MAX_DEVICE_COUNT 1
#define MAX_TARGET_COUNT 1

bool pn53x_get_reg(nfc_device_t* pnd, uint16_t ui16Reg, uint8_t* ui8Value);

void
test_register_endianness (void)
{
    nfc_device_desc_t devices[MAX_DEVICE_COUNT];
    size_t device_count;
    bool res;

    nfc_list_devices (devices, MAX_DEVICE_COUNT, &device_count);
    if (!device_count)
	cut_omit ("No NFC device found");

    nfc_device_t *device;

    device = nfc_connect (&(devices[0]));
    cut_assert_not_null (device, cut_message ("nfc_connect"));

    uint8_t value;

    /* Read valid XRAM memory */
    res = pn53x_get_reg (device, 0xF0FF, &value);
    cut_assert_true (res, cut_message ("read register 0xF0FF"));

    /* Read invalid SFR register */
    res = pn53x_get_reg (device, 0xFFF0, &value);
    cut_assert_false (res, cut_message ("read register 0xFFF0"));

    nfc_disconnect (device);
}
