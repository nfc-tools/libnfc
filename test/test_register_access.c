#include <cutter.h>

#include <nfc/nfc.h>
#include "chips/pn53x.h"

#define MAX_DEVICE_COUNT 1
#define MAX_TARGET_COUNT 1

void test_register_access(void);

void
test_register_access(void)
{
  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  int res = 0;

  nfc_context *context;
  nfc_init(&context);

  size_t device_count = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
  if (!device_count)
    cut_omit("No NFC device found");

  nfc_device *device;

  device = nfc_open(context, connstrings[0]);
  cut_assert_not_null(device, cut_message("nfc_open"));

  uint8_t value;

  /* Set a 0xAA test value in writable register memory to test register access */
  res = pn53x_write_register(device, PN53X_REG_CIU_TxMode, 0xFF, 0xAA);
  cut_assert_equal_int(0, res, cut_message("write register value to 0xAA"));

  /* Get test value from register memory */
  res = pn53x_read_register(device, PN53X_REG_CIU_TxMode, &value);
  cut_assert_equal_int(0, res, cut_message("read register value"));
  cut_assert_equal_uint(0xAA, value, cut_message("check register value"));

  /* Set a 0x55 test value in writable register memory to test register access */
  res = pn53x_write_register(device, PN53X_REG_CIU_TxMode, 0xFF, 0x55);
  cut_assert_equal_int(0, res, cut_message("write register value to 0x55"));

  /* Get test value from register memory */
  res = pn53x_read_register(device, PN53X_REG_CIU_TxMode, &value);
  cut_assert_equal_int(0, res, cut_message("read register value"));
  cut_assert_equal_uint(0x55, value, cut_message("check register value"));

  nfc_close(device);
  nfc_exit(context);
}
