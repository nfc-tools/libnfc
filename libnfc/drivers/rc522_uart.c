/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file rc522_uart.c
 * @brief Driver for MFRC522- and FM17222-based devices connected with an UART
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "rc522_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/rc522.h"
#include "uart.h"

#define RC522_UART_BOOT_SPEED 9600
#define RC522_UART_DEFAULT_SPEED 115200
#define RC522_UART_DRIVER_NAME "rc522_uart"
#define RC522_UART_IO_TIMEOUT 50

#define LOG_CATEGORY "libnfc.driver.rc522_uart"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

// Internal data structs
const struct rc522_io rc522_uart_io;
struct rc522_uart_data {
	serial_port port;
	uint32_t baudrate;
};

#define DRIVER_DATA(pnd) ((struct rc522_uart_data*)(pnd->driver_data))
/*
int rc522_uart_wakeup(struct nfc_device * pnd) {
	int ret;

	// High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for RC522 being wakeup.
	const uint8_t rc522_wakeup_preamble[] = { 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	ret = uart_send(DRIVER_DATA(pnd)->port, rc522_wakeup_preamble, sizeof(rc522_wakeup_preamble), RC522_UART_IO_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	return rc522_wait_wakeup(pnd);
}
*/

void rc522_uart_close(nfc_device * pnd) {
	rc522_powerdown(pnd);
	// Release UART port
	uart_close(DRIVER_DATA(pnd)->port);
	rc522_data_free(pnd);
	nfc_device_free(pnd);
}

bool rc522_uart_test_baudrate(struct nfc_device * pnd, uint32_t baudrate) {
	int ret;

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Attempting to establish a connection at %d bps.", baudrate);

	// Update UART baudrate
	if ((ret = uart_set_speed(DRIVER_DATA(pnd)->port, baudrate)) < 0) {
		return false;
	}

	// Attempt to test and initialize the device
	if (rc522_init(pnd) != NFC_SUCCESS) {
		return false;
	}

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Connection with a RC522 at %d bps established successfully.", baudrate);

	return true;
}

int rc522_uart_create(const nfc_context * context, const nfc_connstring connstring, const char * portPath, uint32_t userBaudRate, struct nfc_device ** pndPtr) {
	int ret;
	serial_port sp;

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Attempt to open: %s.", portPath);
	sp = uart_open(portPath);
	if (sp == INVALID_SERIAL_PORT) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Invalid serial port: %s", portPath);
		return NFC_EIO;
	}
	if (sp == CLAIMED_SERIAL_PORT) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Serial port already claimed: %s", portPath);
		return NFC_EIO;
	}

	// We need to flush input to be sure first reply does not comes from older byte transceive
	if ((ret = uart_flush_input(sp, true)) < 0) {
		return ret;
	}

	nfc_device * pnd = nfc_device_new(context, connstring);
	if (!pnd) {
		perror("nfc_device_new");
		uart_close(sp);
		return NFC_ESOFT;
	}
	pnd->driver = &rc522_uart_driver;

	pnd->driver_data = malloc(sizeof(struct rc522_uart_data));

	if (!pnd->driver_data) {
		perror("malloc");
		uart_close(sp);
		nfc_device_free(pnd);
		return NFC_ESOFT;
	}
	DRIVER_DATA(pnd)->port = sp;
	DRIVER_DATA(pnd)->baudrate = userBaudRate;

	// Alloc and init chip's data
	if (rc522_data_new(pnd, &rc522_uart_io)) {
		perror("rc522_data_new");
		uart_close(sp);
		nfc_device_free(pnd);
		return NFC_ESOFT;
	}

	// Here we'll have to address several posibilities:
	// - The hard reset trick did the work, and the RC522 is up and listening at 9600
	// - The hard reset didn't work, but the RC522 hasn't been used yet and therefore listens at 9600
	// - The hard reset didn't work and the RC522 is not using the default, so we'll use the custom provided baud rate

	// Let's try first with boot baud rate
	if (
			!rc522_uart_test_baudrate(pnd, RC522_UART_BOOT_SPEED) &&
			!rc522_uart_test_baudrate(pnd, userBaudRate)
	) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Could not connect with RC522 at %d or %d bps.", RC522_UART_BOOT_SPEED, userBaudRate);
		rc522_uart_close(pnd);
		return NFC_EIO;
	}

	*pndPtr = pnd;
	return NFC_SUCCESS;
}

size_t rc522_uart_scan(const nfc_context * context, nfc_connstring connstrings[], const size_t connstrings_len) {
	size_t device_found = 0;
	char ** acPorts = uart_list_ports();
	const char * acPort;
	size_t iDevice = 0;

	while ((acPort = acPorts[iDevice++])) {
		nfc_connstring connstring;
		snprintf(connstring, sizeof(nfc_connstring), "%s:%s:%"PRIu32, RC522_UART_DRIVER_NAME, acPort, RC522_UART_DEFAULT_SPEED);

		nfc_device * pnd;
		int ret = rc522_uart_create(context, connstring, acPort, RC522_UART_DEFAULT_SPEED, &pnd);
		if (ret == NFC_ESOFT) {
			uart_list_free(acPorts);
			return 0;
		}
		if (ret != NFC_SUCCESS) {
			continue;
		}
		rc522_uart_close(pnd);

		memcpy(connstrings[device_found], connstring, sizeof(nfc_connstring));
		device_found++;

		// Test if we reach the maximum "wanted" devices
		if (device_found >= connstrings_len)
			break;
    }

	uart_list_free(acPorts);
	return device_found;
}

struct nfc_device * rc522_uart_open(const nfc_context * context, const nfc_connstring connstring) {
	char * port_str = NULL;
	char * baud_str = NULL;
	uint32_t baudrate;
	char * endptr;
	struct nfc_device * pnd = NULL;

	int decodelvl = connstring_decode(connstring, RC522_UART_DRIVER_NAME, NULL, &port_str, &baud_str);
	switch (decodelvl) {
		case 2: // Got port but no speed
			baudrate = RC522_UART_DEFAULT_SPEED;
			break;

		case 3: // Got port and baud rate
			// TODO: set baud rate AFTER initialization
			baudrate = (uint32_t) strtol(baud_str, &endptr, 10);
			if (*endptr != '\0') {
				free(port_str);
				free(baud_str);
				return NULL;
			}

			free(baud_str);
			break;

		default: // Got unparseable gibberish
			free(port_str);
			free(baud_str);
			return NULL;
    }

	rc522_uart_create(context, connstring, port_str, baudrate, &pnd);
	free(port_str);
	return pnd;
}

#define READ 1
#define WRITE 0
uint8_t rc522_uart_pack(int reg, int op) {
	assert(reg < 64);
	assert(op == READ || op == WRITE);

	return op << 7 | reg;
}

int rc522_uart_read(struct nfc_device * pnd, uint8_t reg, uint8_t * data, size_t size) {
	uint8_t cmd = rc522_uart_pack(reg, READ);
	int ret;

	while (size > 0) {
		if ((ret = uart_send(DRIVER_DATA(pnd)->port, &cmd, 1, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		if ((ret = uart_receive(DRIVER_DATA(pnd)->port, data, 1, NULL, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		size--;
		data++;
	}

	return NFC_SUCCESS;

error:
	uart_flush_input(DRIVER_DATA(pnd)->port, true);
	return ret;
}

int rc522_uart_write(struct nfc_device * pnd, uint8_t reg, const uint8_t * data, size_t size) {
	uint8_t cmd = rc522_uart_pack(reg, WRITE);
	int ret;

	while (size > 0) {
		// First: send write request
		if ((ret = uart_send(DRIVER_DATA(pnd)->port, &cmd, 1, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		// Second: wait for a reply
		uint8_t reply;
		if ((ret = uart_receive(DRIVER_DATA(pnd)->port, &reply, 1, NULL, RC522_UART_IO_TIMEOUT)) < 0) {
			return ret;
		}

		// Third: compare sent and received. They must match.
		if (cmd != reply) {
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "rc522_uart_write ack does not match (sent %02X, received %02X)", cmd, reply);
			ret = NFC_ECHIP;
			goto error;
		}

		// Fourth: send register data
		if ((ret = uart_send(DRIVER_DATA(pnd)->port, data, 1, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		size--;
		data++;
	}

	return NFC_SUCCESS;

error:
	uart_flush_input(DRIVER_DATA(pnd)->port, true);
	return ret;
}

int rc522_uart_reset_baud_rate(struct nfc_device * pnd) {
	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Restoring baud rate to default of %d bps.", RC522_UART_BOOT_SPEED);
	return uart_set_speed(DRIVER_DATA(pnd)->port, RC522_UART_BOOT_SPEED);
}

int rc522_uart_upgrade_baud_rate(struct nfc_device * pnd) {
	uint32_t userBaudRate = DRIVER_DATA(pnd)->baudrate;
	if (userBaudRate == RC522_UART_BOOT_SPEED) {
		return NFC_SUCCESS;
	}

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Upgrading baud rate to user-specified %d bps.", userBaudRate);
	return
			uart_set_speed(DRIVER_DATA(pnd)->port, userBaudRate) ||
			rc522_send_baudrate(pnd, userBaudRate);
}

const struct rc522_io rc522_uart_io = {
	.read	= rc522_uart_read,
	.write	= rc522_uart_write,
	.reset_baud_rate	= rc522_uart_reset_baud_rate,
	.upgrade_baud_rate	= rc522_uart_upgrade_baud_rate,
};

const struct nfc_driver rc522_uart_driver = {
	.name								= RC522_UART_DRIVER_NAME,
	.scan_type							= INTRUSIVE,
	.scan								= rc522_uart_scan,
	.open								= rc522_uart_open,
	.close								= rc522_uart_close,
//	.strerror							= rc522_strerror,

//	.initiator_init						= rc522_initiator_init,
	// MFRC522 has no secure element
	.initiator_init_secure_element		= NULL,
//	.initiator_select_passive_target	= rc522_initiator_select_passive_target,
//	.initiator_poll_target				= rc522_initiator_poll_target,
	.initiator_select_dep_target		= NULL,
//	.initiator_deselect_target			= rc522_initiator_deselect_target,
//	.initiator_transceive_bytes			= rc522_initiator_transceive_bytes,
//	.initiator_transceive_bits			= rc522_initiator_transceive_bits,
//	.initiator_transceive_bytes_timed	= rc522_initiator_transceive_bytes_timed,
//	.initiator_transceive_bits_timed	= rc522_initiator_transceive_bits_timed,
//	.initiator_target_is_present		= rc522_initiator_target_is_present,

	// MFRC522 is unable to work as target
	.target_init					 	= NULL,
	.target_send_bytes		 			= NULL,
	.target_receive_bytes				= NULL,
	.target_send_bits					= NULL,
	.target_receive_bits	 			= NULL,

	.device_set_property_bool			= rc522_set_property_bool,
	.device_set_property_int			= rc522_set_property_int,
	.get_supported_modulation			= rc522_get_supported_modulation,
	.get_supported_baud_rate			= rc522_get_supported_baud_rate,
//	.device_get_information_about		= rc522_get_information_about,

	.abort_command						= rc522_abort,
//	.idle								= rc522_idle,
	.powerdown							= rc522_powerdown,
};

