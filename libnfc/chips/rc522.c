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

#include <stdlib.h>

#include "rc522.h"
#include "rc522-internal.h"

#include "nfc/nfc.h"
#include "nfc-internal.h"
#include "timing.h"
 
#define LOG_CATEGORY "libnfc.chip.rc522"
#define LOG_GROUP NFC_LOG_GROUP_CHIP

const nfc_modulation_type rc522_initiator_modulation[] = { NMT_ISO14443A, 0 };
const nfc_modulation_type rc522_target_modulation[] = { 0 };

const nfc_baud_rate rc522_iso14443a_supported_baud_rates[] = { NBR_847, NBR_424, NBR_212, NBR_106, 0 };

struct rc522_chip_data {
	const struct rc522_io * io;
	rc522_type version;
};

#define CHIP_DATA(x) ((struct rc522_chip_data *) (x)->chip_data)

int rc522_data_new(struct nfc_device * pnd, const struct rc522_io * io) {
	pnd->chip_data = malloc(sizeof(struct rc522_chip_data));
	if (!pnd->chip_data) {
		perror("malloc");
		return NFC_ESOFT;
	}

	CHIP_DATA(pnd)->io = io;
	CHIP_DATA(pnd)->version = RC522_UNKNOWN;
	return NFC_SUCCESS;
}

void rc522_data_free(struct nfc_device * pnd) {
	free(pnd->chip_data);
}

int rc522_read_bulk(struct nfc_device * pnd, uint8_t reg, uint8_t * val, size_t len) {
	int ret = CHIP_DATA(pnd)->io->read(pnd, reg, val, len);
	if (ret) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to read register %02X!", reg);
		return ret;
	}

#ifdef LOG
	char action[8];
	snprintf(action, sizeof(action), "RD %02X", reg);
	LOG_HEX(NFC_LOG_GROUP_CHIP, action, val, len);
#endif

	return NFC_SUCCESS;
}

int rc522_write_bulk(struct nfc_device * pnd, uint8_t reg, const uint8_t * val, size_t len) {
	int ret = CHIP_DATA(pnd)->io->write(pnd, reg, val, len);
	if (ret) {
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to write register %02X!", reg);
		return ret;
	}

#ifdef LOG
	char action[8];
	snprintf(action, sizeof(action), "WR %02X", reg);
	LOG_HEX(NFC_LOG_GROUP_CHIP, action, val, len);
#endif

	return NFC_SUCCESS;
}

int rc522_read_reg(struct nfc_device * pnd, uint8_t reg) {
	uint8_t val;
	int ret;

	if ((ret = rc522_read_bulk(pnd, reg, &val, 1)) < 0) {
		return ret;
	}

	return val;
}

int rc522_write_reg(struct nfc_device * pnd, uint8_t reg, uint8_t val, uint8_t mask) {
	if (mask != 0xFF) {
		int oldval = rc522_read_reg(pnd, reg);
		if (oldval < 0) {
			return oldval;
		}

		val = (val & mask) | (oldval & ~mask);
	}

	return rc522_write_bulk(pnd, reg, &val, 1);
}

int rc522_start_command(struct nfc_device * pnd, rc522_cmd cmd) {
	bool needsRX = false;

	// Disabling RX saves energy, so based on the command we'll also update the RxOff flag
	switch (cmd) {
		case CMD_IDLE:
		case CMD_MEM:
		case CMD_GENRANDOMID:
		case CMD_CALCCRC:
		case CMD_TRANSMIT:
		case CMD_SOFTRESET:
			break;

		case CMD_RECEIVE:
		case CMD_TRANSCEIVE:
		case CMD_MFAUTHENT:
			needsRX = true;
			break;

		case CMD_NOCMDCHANGE:
			return NFC_SUCCESS;

		default:
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Attempted to execute non-existant command: %02X", cmd);
			pnd->last_error = NFC_ESOFT;
			return pnd->last_error;
	}

	uint8_t regval = cmd;
	if (!needsRX) {
		regval |= REG_CommandReg_RcvOff;
	}

	return rc522_write_reg(pnd, REG_CommandReg, regval, 0xFF);
}

int rc522_wait_wakeup(struct nfc_device * pnd) {
	// NXP does not mention in the datasheet how much time does it take for RC522 to come back to life, so we'll wait up to 50ms
	timeout_t to;
	timeout_init(&to, 50);

	// rc522_read_reg updates last_error. Backup it to ignore timeouts
	int last_error = pnd->last_error;

	while (timeout_check(&to)) {
		int ret = rc522_read_reg(pnd, REG_CommandReg);
		if (ret < 0 && ret != NFC_ETIMEOUT) {
			return ret;
		}

		// If the powerdown bit is zero the RC522 is ready to kick asses!
		if ((ret & REG_CommandReg_PowerDown) == 0) {
			pnd->last_error = last_error;
			return NFC_SUCCESS;
		}
	}

	log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "rc522_wait_wakeup timeout!");
	pnd->last_error = NFC_ETIMEOUT;
	return pnd->last_error;
}

int rc522_soft_reset(struct nfc_device * pnd) {
	return
			rc522_start_command(pnd, CMD_SOFTRESET) ||
			rc522_wait_wakeup(pnd);
}

int rc522_set_baud_rate(struct nfc_device * pnd, nfc_baud_rate speed) {
	uint8_t txVal, rxVal;

	switch (speed) {
		case NBR_106:
			txVal = REG_TxModeReg_TxSpeed_106k;
			rxVal = REG_RxModeReg_RxSpeed_106k;
			break;

		case NBR_212:
			txVal = REG_TxModeReg_TxSpeed_212k;
			rxVal = REG_RxModeReg_RxSpeed_212k;
			break;

		case NBR_424:
			txVal = REG_TxModeReg_TxSpeed_424k;
			rxVal = REG_RxModeReg_RxSpeed_424k;
			break;

		case NBR_847:
			txVal = REG_TxModeReg_TxSpeed_847k;
			rxVal = REG_RxModeReg_RxSpeed_847k;
			break;

		default:
			return NFC_EINVARG;
	}

	return
			rc522_write_reg(pnd, REG_TxModeReg, txVal, REG_TxModeReg_TxSpeed_MASK) ||
			rc522_write_reg(pnd, REG_RxModeReg, rxVal, REG_RxModeReg_RxSpeed_MASK);
}

int rc522_initiator_select_passive_target_ext(struct nfc_device * pnd, const nfc_modulation nm, const uint8_t * pbtInitData, const size_t szInitData, nfc_target * pnt, int timeout)
{
	int ret;

	if (nm.nmt != NMT_ISO14443A) {
		return NFC_EINVARG;
	}

	ret = rc522_set_baud_rate(pnd, nm.nbr);
	if (ret < 0) {
		return ret;
	}

	// TODO
	return NFC_ENOTIMPL;
}

int rc522_get_supported_modulation(struct nfc_device * pnd, const nfc_mode mode, const nfc_modulation_type ** const supported_mt) {
	switch (mode) {
		case N_INITIATOR:
			*supported_mt = rc522_initiator_modulation;
			break;

		case N_TARGET:
			*supported_mt = rc522_target_modulation;
			break;

		default:
			return NFC_EINVARG;
	}

	return NFC_SUCCESS;
}

int rc522_get_supported_baud_rate(struct nfc_device * pnd, const nfc_mode mode, const nfc_modulation_type nmt, const nfc_baud_rate ** const supported_br) {
	switch (mode) {
		case N_INITIATOR:
			switch (nmt) {
				case NMT_ISO14443A:
					*supported_br = rc522_iso14443a_supported_baud_rates;
					break;

				default:
					return NFC_EINVARG;
			}
			break;

		case N_TARGET:
		default:
			return NFC_EINVARG;
	}

	return NFC_SUCCESS;
}

int rc522_set_property_bool(struct nfc_device * pnd, const nfc_property property, const bool enable) {
	int ret;

	switch (property) {
		case NP_HANDLE_CRC:
			if (pnd->bCrc == enable) {
				return NFC_SUCCESS;
			}

			ret =
					rc522_write_reg(pnd, REG_TxModeReg, enable ? ~0 : 0, REG_TxModeReg_TxCRCEn) ||
					rc522_write_reg(pnd, REG_RxModeReg, enable ? ~0 : 0, REG_RxModeReg_RxCRCEn);
			if (ret) {
				return ret;
			}

			pnd->bCrc = enable;
			return NFC_SUCCESS;

		case NP_HANDLE_PARITY:
			if (pnd->bPar == enable) {
				return NFC_SUCCESS;
			}

			ret = rc522_write_reg(pnd, REG_MfRxReg, enable ? 0 : ~0, REG_MfRxReg_ParityDisable);
			if (ret) {
				return ret;
			}

			pnd->bPar = enable;
			return NFC_SUCCESS;

		case NP_EASY_FRAMING:
			pnd->bEasyFraming = enable;
			return NFC_SUCCESS;

		case NP_ACTIVATE_FIELD:
			return rc522_write_reg(pnd, REG_TxControlReg, enable ? ~0 : 0, REG_TxControlReg_Tx2RFEn | REG_TxControlReg_Tx1RFEn);
		
		case NP_ACTIVATE_CRYPTO1:
			return rc522_write_reg(pnd, REG_Status2Reg, enable ? ~0 : 0, REG_Status2Reg_MFCrypto1On);

		case NP_FORCE_ISO14443_A:
			// ISO14443-A is the only mode supported by MFRC522
			return NFC_SUCCESS;

		case NP_FORCE_SPEED_106:
			if (!enable) {
				return NFC_SUCCESS;
			}

			int ret = rc522_set_baud_rate(pnd, NBR_106);
			if (ret) {
				pnd->last_error = ret;
			}
			return ret;

		case NP_ACCEPT_MULTIPLE_FRAMES:
		case NP_AUTO_ISO14443_4:
		case NP_ACCEPT_INVALID_FRAMES:
		case NP_INFINITE_SELECT:
		case NP_FORCE_ISO14443_B:
		case NP_TIMEOUT_COMMAND:
		case NP_TIMEOUT_ATR:
		case NP_TIMEOUT_COM:
			pnd->last_error = NFC_EINVARG;
			return NFC_EINVARG;
	}

	pnd->last_error = NFC_EINVARG;
	return NFC_EINVARG;
}

int rc522_set_property_int(struct nfc_device * pnd, const nfc_property property, const int value) {
	// TODO
	return NFC_ENOTIMPL;
}

int rc522_abort(struct nfc_device * pnd) {
	return rc522_start_command(pnd, CMD_IDLE);
}

int rc522_powerdown(struct nfc_device * pnd) {
	return rc522_write_reg(pnd, REG_CommandReg, REG_CommandReg_RcvOff | REG_CommandReg_PowerDown | CMD_IDLE, 0xFF);
}

// NXP MFRC522 datasheet section 16.1.1
const uint8_t MFRC522_V1_SELFTEST[FIFO_SIZE] = {
	0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C, 0xC2, 0xD8, 0x7C, 0x4D, 0xD9, 0x70, 0xC7, 0x73,
	0x10, 0xE6, 0xD2, 0xAA, 0x5E, 0xA1, 0x3E, 0x5A, 0x14, 0xAF, 0x30, 0x61, 0xC9, 0x70, 0xDB, 0x2E,
	0x64, 0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC, 0x22, 0xBC, 0xD3, 0x72, 0x35, 0xCD, 0xAA, 0x41,
	0x1F, 0xA7, 0xF3, 0x53, 0x14, 0xDE, 0x7E, 0x02, 0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79
};
const uint8_t MFRC522_V2_SELFTEST[FIFO_SIZE] = {
	0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95, 0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE,
	0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82, 0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49,
	0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81, 0x5D, 0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9,
	0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B, 0x6D, 0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F
};

// Extracted from a FM17522 with version 0x88. Fudan Semiconductor datasheet does not include it, though.
const uint8_t FM17522_SELFTEST[FIFO_SIZE] = {
	0x00, 0xD6, 0x78, 0x8C, 0xE2, 0xAA, 0x0C, 0x18, 0x2A, 0xB8, 0x7A, 0x7F, 0xD3, 0x6A, 0xCF, 0x0B,
	0xB1, 0x37, 0x63, 0x4B, 0x69, 0xAE, 0x91, 0xC7, 0xC3, 0x97, 0xAE, 0x77, 0xF4, 0x37, 0xD7, 0x9B,
	0x7C, 0xF5, 0x3C, 0x11, 0x8F, 0x15, 0xC3, 0xD7, 0xC1, 0x5B, 0x00, 0x2A, 0xD0, 0x75, 0xDE, 0x9E,
	0x51, 0x64, 0xAB, 0x3E, 0xE9, 0x15, 0xB5, 0xAB, 0x56, 0x9A, 0x98, 0x82, 0x26, 0xEA, 0x2A, 0x62
};

int rc522_self_test(struct nfc_device * pnd) {
	int version = rc522_read_reg(pnd, REG_VersionReg);
	if (version < 0) {
		return version;
	}

	const uint8_t * correct;
	switch (version) {
		case MFRC522_V1:
			correct = MFRC522_V1_SELFTEST;
			break;

		case MFRC522_V2:
			correct = MFRC522_V2_SELFTEST;
			break;

		case FM17522:
			correct = FM17522_SELFTEST;
			break;

		default:
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unknown chip version: 0x%02X", version);
			return NFC_ECHIP;
	}

	int ret;
	uint8_t zeroes[25];
	memset(zeroes, 0x00, sizeof(zeroes));
	// MFRC522 datasheet section 16.1.1
	ret =
			// 1. Perform a soft reset
			rc522_soft_reset(pnd) ||
			// 2. Clear the internal buffer by writing 25 bytes of 0x00 and execute the Mem command
			rc522_write_bulk(pnd, REG_FIFODataReg, zeroes, sizeof(zeroes)) ||
			rc522_start_command(pnd, CMD_MEM) ||
			// 3. Enable the self test by writing 0x09 to the AutoTestReg register
			rc522_write_reg(pnd, REG_AutoTestReg, REG_AutoTestReg_SelfTest_Enabled, REG_AutoTestReg_SelfTest_MASK) ||
			// 4. Write 0x00h to the FIFO buffer
			rc522_write_reg(pnd, REG_FIFODataReg, 0x00, 0xFF) ||
			// 5. Start the self test with the CalcCRC command
			rc522_start_command(pnd, CMD_CALCCRC);
	if (ret) {
		return ret;
	}

	// 6. Wait for the RC522 to calculate the selftest values
	// The official datasheet does not mentions how much time does it take, let's use 5ms
	timeout_t to;
	timeout_init(&to, 5);

	while (1) {
		if (!timeout_check(&to)) {
			return NFC_ETIMEOUT;
		}

		if ((ret = rc522_read_reg(pnd, REG_DivIrqReg)) < 0) {
			return ret;
		}

		// If the RC522 has finished calculating the CRC proceed
		if (ret & REG_DivIrqReg_CRCIRq) {
			break;
		}
	}

	uint8_t response[FIFO_SIZE];
	ret =
			// 7. Read selftest result
			rc522_read_bulk(pnd, REG_FIFODataReg, response, FIFO_SIZE) ||
			// 8. Disable selftest operation mode
			rc522_write_reg(pnd, REG_AutoTestReg, REG_AutoTestReg_SelfTest_Disabled, REG_AutoTestReg_SelfTest_MASK);
	if (ret) {
		return ret;
	}

	if (memcmp(correct, response, FIFO_SIZE) != 0) {
		return NFC_ECHIP;
	}

	CHIP_DATA(pnd)->version = version;
	return NFC_SUCCESS;
}

