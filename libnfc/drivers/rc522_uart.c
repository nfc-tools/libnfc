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

#define RC522_UART_DEFAULT_SPEED 9600
#define RC522_UART_DRIVER_NAME "rc522_uart"
#define RC522_UART_IO_TIMEOUT 50

#define LOG_CATEGORY "libnfc.driver.rc522_uart"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

// Internal data structs
const struct rc522_io rc522_uart_io;
struct rc522_uart_data {
  serial_port port;
};

#define DRIVER_DATA(pnd) ((struct rc522_uart_data*)(pnd->driver_data))

void rc522_uart_close(nfc_device * pnd) {
//	rc522_idle(pnd);

	// Release UART port
	uart_close(DRIVER_DATA(pnd)->port);
	rc522_data_free(pnd);
	nfc_device_free(pnd);
}

size_t rc522_uart_scan(const nfc_context * context, nfc_connstring connstrings[], const size_t connstrings_len) {
	size_t device_found = 0;
	serial_port sp;
	char ** acPorts = uart_list_ports();
	const char * acPort;
	size_t iDevice = 0;

	while ((acPort = acPorts[iDevice++])) {
		sp = uart_open(acPort);
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Trying to find RC522 device on serial port: %s at %d baud.", acPort, RC522_UART_DEFAULT_SPEED);

		if (sp == INVALID_SERIAL_PORT || sp == CLAIMED_SERIAL_PORT) {
			continue;
		}

		// We need to flush input to be sure first reply does not comes from older byte transceive
		uart_flush_input(sp, true);
		// Serial port claimed but we need to check if a RC522_UART is opened.
		uart_set_speed(sp, RC522_UART_DEFAULT_SPEED);

		nfc_connstring connstring;
		snprintf(connstring, sizeof(nfc_connstring), "%s:%s:%"PRIu32, RC522_UART_DRIVER_NAME, acPort, RC522_UART_DEFAULT_SPEED);
		nfc_device * pnd = nfc_device_new(context, connstring);
		if (!pnd) {
			perror("nfc_device_new");
			uart_close(sp);
			nfc_device_free(pnd);
			uart_list_free(acPorts);
			return 0;
		}
		pnd->driver = &rc522_uart_driver;

		pnd->driver_data = malloc(sizeof(struct rc522_uart_data));
		if (!pnd->driver_data) {
			perror("malloc");
			uart_close(sp);
			nfc_device_free(pnd);
			uart_list_free(acPorts);
			return 0;
		}
		DRIVER_DATA(pnd)->port = sp;

		// Alloc and init chip's data
		if (rc522_data_new(pnd, &rc522_uart_io)) {
			perror("rc522_data_new");
			uart_close(sp);
			nfc_device_free(pnd);
			uart_list_free(acPorts);
			return 0;
		}

		// Check communication using self test
		int res = rc522_self_test(pnd);
		rc522_uart_close(pnd);
		if (res < 0) {
			continue;
		}

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
	char * port_str;
	char * baud_str;
	uint32_t baudrate;
	char * endptr;

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

	serial_port sp;
	struct nfc_device * pnd = NULL;

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Attempt to open: %s at %d baud.", port_str, baudrate);
	sp = uart_open(port_str);

	if (sp == INVALID_SERIAL_PORT) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Invalid serial port: %s", port_str);
		free(port_str);
		return NULL;
	}

	if (sp == CLAIMED_SERIAL_PORT) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Serial port already claimed: %s", port_str);
		free(port_str);
		return NULL;
	}

	// We need to flush input to be sure first reply does not comes from older byte transceive
	uart_flush_input(sp, true);
	uart_set_speed(sp, baudrate);

	// We have a connection
	pnd = nfc_device_new(context, connstring);
	if (!pnd) {
		perror("nfc_device_new");
		free(port_str);
		uart_close(sp);
		return NULL;
	}

	snprintf(pnd->name, sizeof(pnd->name), "%s:%s", RC522_UART_DRIVER_NAME, port_str);
	free(port_str);
	pnd->driver = &rc522_uart_driver;
	pnd->driver_data = malloc(sizeof(struct rc522_uart_data));
	if (!pnd->driver_data) {
		perror("malloc");
		uart_close(sp);
		nfc_device_free(pnd);
		return NULL;
	}
	DRIVER_DATA(pnd)->port = sp;

	// Alloc and init chip's data
	if (!rc522_data_new(pnd, &rc522_uart_io)) {
		perror("rc522_data_new");
		uart_close(sp);
		nfc_device_free(pnd);
		return NULL;
	}

	if (rc522_self_test(pnd)) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "rc522_self_test error");
		rc522_uart_close(pnd);
		return NULL;
	}

	return pnd;
}

int rc522_uart_wakeup(struct nfc_device * pnd) {
	int ret;

	/* High Speed Unit (HSU) wake up consist to send 0x55 and wait a "long" delay for RC522 being wakeup. */
	const uint8_t rc522_wakeup_preamble[] = { 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	if ((ret = uart_send(DRIVER_DATA(pnd)->port, rc522_wakeup_preamble, sizeof(rc522_wakeup_preamble), 0)) < 0) {
		return ret;
	}

	return rc522_wait_wakeup(pnd);
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
		if ((ret = uart_send(pnd->driver_data, &cmd, 1, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		if ((ret = uart_receive(pnd->driver_data, data, 1, NULL, RC522_UART_IO_TIMEOUT)) < 0) {
			goto error;
		}

		size--;
		data++;
	}

	return NFC_SUCCESS;

error:
	uart_flush_input(DRIVER_DATA(pnd)->port, true);
	return pnd->last_error;
}

int rc522_uart_write(struct nfc_device * pnd, uint8_t reg, const uint8_t * data, size_t size) {
	uint8_t cmd = rc522_uart_pack(reg, WRITE);

	while (size > 0) {
		// First: send write request
		pnd->last_error = uart_send(pnd->driver_data, &cmd, 1, RC522_UART_IO_TIMEOUT);
		if (pnd->last_error < 0) {
			goto error;
		}

		// Second: wait for a reply
		uint8_t reply;
		pnd->last_error = uart_receive(pnd->driver_data, &reply, 1, NULL, RC522_UART_IO_TIMEOUT);
		if (pnd->last_error < 0) {
			return pnd->last_error;
		}

		// Third: compare sent and received. They must match.
		if (cmd != reply) {
			pnd->last_error = NFC_EIO;
			goto error;
		}

		// Fourth: send register data
		pnd->last_error = uart_send(pnd->driver_data, data, 1, RC522_UART_IO_TIMEOUT);
		if (pnd->last_error < 0) {
			goto error;
		}

		size--;
		data++;
	}

	return NFC_SUCCESS;

error:
	uart_flush_input(DRIVER_DATA(pnd)->port, true);
	return pnd->last_error;
}

const struct rc522_io rc522_uart_io = {
	.read	= rc522_uart_read,
	.write	= rc522_uart_write,
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

