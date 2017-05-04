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
 *
 */

#ifndef __NFC_CHIPS_RC522_H__
#define __NFC_CHIPS_RC522_H__

#include <stdint.h>
#include <nfc/nfc-types.h>

struct rc522_io {
	int (*read)(struct nfc_device * pnd, uint8_t reg, uint8_t * data, size_t size);
	int (*write)(struct nfc_device * pnd, uint8_t reg, const uint8_t * data, size_t size);
	int (*reset_baud_rate)(struct nfc_device * pnd);
	int (*upgrade_baud_rate)(struct nfc_device * pnd);
};

int rc522_data_new(struct nfc_device * pnd, const struct rc522_io * io);
void rc522_data_free(struct nfc_device * pnd);
int rc522_send_baudrate(struct nfc_device * pnd, uint32_t baudrate);
int rc522_init(struct nfc_device * pnd);

int rc522_initiator_init(nfc_device * pnd);
int rc522_initiator_transceive_bits(struct nfc_device * pnd, const uint8_t * txData, const size_t txBits, const uint8_t * pbtTxPar, uint8_t * rxData, uint8_t * pbtRxPar);
int rc522_initiator_transceive_bytes(struct nfc_device * pnd, const uint8_t * txData, const size_t txSize, uint8_t * rxData, const size_t rxMaxBytes, int timeout);
int rc522_get_supported_modulation(nfc_device * pnd, const nfc_mode mode, const nfc_modulation_type ** const supported_mt);
int rc522_get_supported_baud_rate(nfc_device * pnd, const nfc_mode mode, const nfc_modulation_type nmt, const nfc_baud_rate ** const supported_br);
int rc522_set_property_bool(struct nfc_device * pnd, const nfc_property property, const bool enable);
int rc522_set_property_int(struct nfc_device * pnd, const nfc_property property, const int value);
int rc522_abort(struct nfc_device * pnd);
int rc522_powerdown(struct nfc_device * pnd);

#endif
