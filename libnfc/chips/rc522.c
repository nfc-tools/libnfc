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
 
#define LOG_CATEGORY "libnfc.chip.rc522"
#define LOG_GROUP NFC_LOG_GROUP_CHIP

#define RC522_TIMEOUT 5

const nfc_modulation_type rc522_initiator_modulation[] = { NMT_ISO14443A, 0 };
const nfc_modulation_type rc522_target_modulation[] = { 0 };

const nfc_baud_rate rc522_iso14443a_supported_baud_rates[] = { NBR_847, NBR_424, NBR_212, NBR_106, 0 };

struct rc522_chip_data {
	const struct rc522_io * io;
};

#define CHIP_DATA(x) ((struct rc522_chip_data *) (x)->chip_data)

int rc522_data_new(struct nfc_device * pnd, const struct rc522_io * io) {
	pnd->chip_data = malloc(sizeof(struct rc522_chip_data));
	if (!pnd->chip_data) {
		perror("malloc");
		return NFC_ESOFT;
	}

	CHIP_DATA(pnd)->io = io;
	return NFC_SUCCESS;
}

int rc522_read_reg(struct nfc_device * pnd, uint8_t reg) {
	uint8_t val;

	int ret = CHIP_DATA(pnd)->io->read(pnd, reg, &val, 1);
	if (ret < 0) {
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

	return CHIP_DATA(pnd)->io->write(pnd, reg, &val, 1);
}

int rc522_set_baud_rate(struct nfc_device * pnd, nfc_baud_rate speed) {
	uint8_t txVal, rxVal;
	int ret;

	switch (speed) {
		case NBR_106:
			txVal = RC522_REG_TxModeReg_TxSpeed_106k;
			rxVal = RC522_REG_RxModeReg_RxSpeed_106k;
			break;

		case NBR_212:
			txVal = RC522_REG_TxModeReg_TxSpeed_212k;
			rxVal = RC522_REG_RxModeReg_RxSpeed_212k;
			break;

		case NBR_424:
			txVal = RC522_REG_TxModeReg_TxSpeed_424k;
			rxVal = RC522_REG_RxModeReg_RxSpeed_424k;
			break;

		case NBR_847:
			txVal = RC522_REG_TxModeReg_TxSpeed_847k;
			rxVal = RC522_REG_RxModeReg_RxSpeed_847k;
			break;

		default:
			return NFC_EINVARG;
	}

	return
			rc522_write_reg(pnd, RC522_REG_TxModeReg, txVal, RC522_REG_TxModeReg_TxSpeed_MASK) ||
			rc522_write_reg(pnd, RC522_REG_RxModeReg, rxVal, RC522_REG_RxModeReg_RxSpeed_MASK);
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

			ret = rc522_write_reg(pnd, RC522_REG_TxModeReg, enable ? ~0 : 0, RC522_REG_TxModeReg_TxCRCEn) ||
					rc522_write_reg(pnd, RC522_REG_RxModeReg, enable ? ~0 : 0, RC522_REG_RxModeReg_RxCRCEn);
			if (ret) {
				return ret;
			}

			pnd->bCrc = enable;
			return NFC_SUCCESS;

		case NP_HANDLE_PARITY:
			if (pnd->bPar == enable) {
				return NFC_SUCCESS;
			}

			ret = rc522_write_reg(pnd, RC522_REG_MfRxReg, enable ? 0 : ~0, RC522_REG_MfRxReg_ParityDisable);
			if (ret) {
				return ret;
			}

			pnd->bPar = enable;
			return NFC_SUCCESS;

		case NP_EASY_FRAMING:
			pnd->bEasyFraming = enable;
			return NFC_SUCCESS;

		case NP_ACTIVATE_FIELD:
			return rc522_write_reg(pnd, RC522_REG_TxControlReg, enable ? ~0 : 0, RC522_REG_TxControlReg_Tx2RFEn | RC522_REG_TxControlReg_Tx1RFEn);
		
		case NP_ACTIVATE_CRYPTO1:
			return rc522_write_reg(pnd, RC522_REG_Status2Reg, enable ? ~0 : 0, RC522_REG_Status2Reg_MFCrypto1On);

		case NP_FORCE_ISO14443_A:
			// ISO14443-A is the only mode supported by MFRC522
			return NFC_SUCCESS;

		case NP_FORCE_SPEED_106:
			if (!enable) {
				return NFC_SUCCESS;
			}

			return rc522_set_baud_rate(pnd, NBR_106);
			
		case NP_AUTO_ISO14443_4:
		case NP_ACCEPT_INVALID_FRAMES:
		case NP_INFINITE_SELECT:
		case NP_FORCE_ISO14443_B:
		case NP_TIMEOUT_COMMAND:
		case NP_TIMEOUT_ATR:
		case NP_TIMEOUT_COM:
			return NFC_EINVARG;
	}

	return NFC_EINVARG;
}

int rc522_set_property_int(struct nfc_device * pnd, const nfc_property property, const int value) {
	// TODO
	return NFC_ENOTIMPL;
}

int rc522_idle(struct nfc_device * pnd) {
	// Set idle and disable RX demodulator to save energy
	return rc522_write_reg(pnd, RC522_REG_CommandReg, RC522_CMD_Idle | RC522_REG_CommandReg_PowerDown, 0xFF);
}

