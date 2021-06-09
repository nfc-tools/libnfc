/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-st25tb-info.c
 * @brief Read ISO-14443-B ST25TB* and legacy SR* cards
 */

/*	Benjamin DELPY `gentilkiwi`
 *	https://blog.gentilkiwi.com
 *	benjamin@gentilkiwi.com
 *	Licence : https://creativecommons.org/licenses/by/4.0/
 *	Rely on : libnfc - https://github.com/nfc-tools/libnfc
 *	
 *	$ gcc -lnfc -o nfc-st25tb-info nfc-st25tb-info.c
 *	$ ./nfc-st25tb-info
*/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <nfc/nfc.h>

bool get_info(const nfc_target *pnt, uint8_t *pnbBlock, uint8_t *pbnSystem);
bool get_block_at(nfc_device *pnd, uint8_t block, uint8_t value[4], bool bPrintIt);
void print_hex(const uint8_t *pbtData, const size_t szBytes);

int main(int argc, const char *argv[])
{
	nfc_context *context = NULL;
	nfc_device *pnd = NULL;
	nfc_target nt = {0};
	nfc_modulation nm = {NMT_ISO14443B, NBR_106};

	int res;
	uint8_t i, nbBlock, bnSystem;

	(void) argc;
	(void) argv;

	nfc_init(&context);
	if(context)
	{
		pnd = nfc_open(context, NULL);
		if(pnd)
		{
			res = nfc_initiator_init(pnd);
			if(res == NFC_SUCCESS)
			{
				printf("Reader  : %s - via %s\n  ...wait for card...\n", nfc_device_get_name(pnd), nfc_device_get_connstring(pnd));

				res = nfc_initiator_list_passive_targets(pnd, nm, &nt, 1);
				if(res == 0) // we don't really wanted a NMT_ISO14443B
				{
					nm.nmt = NMT_ISO14443B2SR; // we want a NMT_ISO14443B2SR, but needed to ask for NMT_ISO14443B before
					if (nfc_initiator_select_passive_target(pnd, nm, NULL, 0, &nt) > 0)
					{
						if(get_info(&nt, &nbBlock, &bnSystem))
						{
							printf("\nData    :\n");
							for(i = 0; i < nbBlock; i++)
							{
								get_block_at(pnd, i, NULL, 1);
							}
							get_block_at(pnd, bnSystem, NULL, 1);
						}
					}
				}
				else if(res > 0)
				{
					printf("ERROR - We got a NMT_ISO14443B ?\n");
				}
				else printf("ERROR - nfc_initiator_list_passive_targets: %i\n", res);
			}
			else printf("ERROR - nfc_initiator_init: %i\n", res);
			
			nfc_close(pnd);
		}
		else printf("ERROR - nfc_open\n");

		nfc_exit(context);
	}
	else printf("ERROR - nfc_init\n");

	return 0;
}

bool get_info(const nfc_target *pnt, uint8_t *pnbBlock, uint8_t *pbnSystem)
{
	bool bRet = false, bIsLegacy = false;
	const uint8_t *p;
	uint8_t chipId;

	*pnbBlock = 0x10;
	*pbnSystem = 0xff;
	
	if(pnt->nm.nmt == NMT_ISO14443B2SR)
	{
		printf("Target  : %s (%s)\nUID     : ", str_nfc_modulation_type(pnt->nm.nmt), str_nfc_baud_rate(pnt->nm.nbr));
		print_hex(pnt->nti.nsi.abtUID, sizeof(pnt->nti.nsi.abtUID));
		printf("\n");
		
		p = pnt->nti.nsi.abtUID;
		if(p[7] == 0xd0) // ST25TB* / SR*
		{
			bRet = true;
			chipId = p[5];
			printf("Manuf   : 0x%02x - %s\nChipId  : 0x%02x - ", p[6], (p[6] == 0x02) ? "STMicroelectronics" : "other", chipId);

			switch(chipId)
			{
				case 0x3f: // 00111111
					printf("ST25TB02K");
					*pnbBlock = 0x40;
					break;
				case 0x1f: // 00011111
					printf("ST25TB04K");
					*pnbBlock = 0x80;
					break;
				case 0x1b: // 00011011
					printf("ST25TB512-AC");
					break;
				case 0x33: // 00110011
					printf("ST25TB512-AT");
					break;
					
				default:
					chipId >>= 2;
					printf("legacy ? - 0x%02x - ", chipId);
					bIsLegacy = 1;
					switch(chipId)
					{
						case 0x02:
							printf("SR176");
							*pnbBlock = 0x0e;
							*pbnSystem = 0x0f;
							break;
						case 0x03:
							printf("SRIX4K");
							*pnbBlock = 0x80;
							break;
						case 0x04:
							printf("SRIX512");
							break;
						case 0x06:
							printf("SRI512");
							break;
						case 0x07:
							printf("SRI4K");
							*pnbBlock = 0x80;
							break;
						case 0x0c:
							printf("SRT512");
							break;
						default:
							bIsLegacy = 0;
							printf("unknown");
					}
			}
			
			printf("\nSerial  : 0x");
			if(bIsLegacy)
			{
				printf("%1x", p[5] & 0x03);
			}
			printf("%02x%02x%02x%02x%02x\n|usr blk: %hhu\n|sys blk: %hhu\n", p[4], p[3], p[2], p[1], p[0], *pnbBlock, *pbnSystem);
		}
		else printf("WARNI - Last byte of UID isn\'t 0xd0, but 0x%02x (not ST25TB / SR series?)\n", p[7]);
	}
	else printf("ERROR - not a NMT_ISO14443B2SR ?\n");
	
	return bRet;
}

bool get_block_at(nfc_device *pnd, uint8_t block, uint8_t value[4], bool bPrintIt)
{
	bool bRet = false;
	uint8_t tx[2] = {0x08, block}, rx[4];	
	int res;
	
	res = nfc_initiator_transceive_bytes(pnd, tx, sizeof(tx), rx, sizeof(rx), 0);
	if(res == 4)
	{
		bRet = true;

		if(value)
		{
			memcpy(value, rx, sizeof(rx));
		}

		if(bPrintIt)
		{
			printf("[%02x] ", block);
			print_hex(rx, sizeof(rx));
			printf("\n");
		}
	}
	else if(res > 0)
	{
		printf("ERROR - We got %i bytes?\n", res);
	}
	else printf("ERROR - nfc_initiator_transceive_bytes: %i\n", res);
	
	return bRet;
}

void print_hex(const uint8_t *pbtData, const size_t szBytes)
{
	size_t szPos;
	for (szPos = 0; szPos < szBytes; szPos++)
	{
		printf("%02x ", pbtData[szPos]);
	}
}