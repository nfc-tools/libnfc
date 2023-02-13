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
 * @file nfc-st25tb.c
 * @brief Tool to operate on ISO-14443-B ST25TB* and legacy SR* cards
 */

/*	Benjamin DELPY `gentilkiwi`
 *	https://blog.gentilkiwi.com
 *	benjamin@gentilkiwi.com
 *	Licence : https://creativecommons.org/licenses/by/4.0/
 *	Rely on : libnfc - https://github.com/nfc-tools/libnfc
 *	
 *	$ gcc -Wall -lnfc -o nfc-st25tb nfc-st25tb.c
 *	$ ./nfc-st25tb -h
 *
 * Tested with
 * - ST25TB512-AC - (BE/Brussels/STIB ; AliExpress ones)
 * - ST25TB512-AT - (FR/Lille/Ilevia ; FR/Reims/Citura ; FR/Dijon/Divia ; FR/Strasbourg/CTS)
 * - SRT512 - legacy - (FR/Bordeaux/TBM)
 * - SRI512 - legacy - (anonymous vending machine)
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H
 
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <nfc/nfc.h>

#if defined(WIN32) /* mingw compiler */
#include <getopt.h>
#endif

#define ST25TB_SR_BLOCK_MAX_SIZE	((uint8_t) 4) // for static arrays
typedef void(*get_info_specific) (uint8_t * systemArea);

typedef struct _st_data {
	uint8_t chipId;
	bool bIsLegacy;
	const char *szName;
	const char *szDatasheetUrl;
	uint8_t blockSize;
	uint8_t nbNormalBlock;
	uint8_t bnSystem;
	get_info_specific pfnGetInfo;
} st_data;

bool get_block_at(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt);
bool set_block_at(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt);
bool set_block_at_confirmed(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt);
const st_data * get_info(const nfc_target *pnt, bool bPrintIt);
void display_system_info(nfc_device *pnd, const st_data * stdata);
void print_hex(const uint8_t *pbtData, const size_t szBytes);

int main(int argc, char *argv[])
{
	nfc_context *context = NULL;
	nfc_device *pnd = NULL;
	nfc_target nt = {0};
	nfc_modulation nm = {NMT_ISO14443B2SR, NBR_106};
	const st_data * stcurrent;
	int opt, res;
	bool bIsBlock = false, bIsRead = false, bIsWrite = false, bIsBadCli = false;
	uint8_t i, blockNumber = 0, data[ST25TB_SR_BLOCK_MAX_SIZE] = {0xff, 0xff, 0xff, 0xff}; // just in case...
	size_t cbData = 0;
	
	while(!bIsBadCli && ((opt = getopt(argc, argv, ":hib:rw:")) != -1))
	{
		switch(opt)
		{
			case 'i':
				
				break;
			
			case 'b':
				if(optarg)
				{
					bIsBlock = true;
					blockNumber = strtoul(optarg, NULL, 0);
				}
				else bIsBadCli = true;

				break;

			case 'r':
				bIsRead = true;
				
				break;

			case 'w':
					if(optarg)
					{
						cbData = strlen(optarg);
						if((cbData == (2*2)) || ((cbData == (4*2))))
						{
							cbData >>= 1;
							if(cbData == 2) // sr176
							{
								res = sscanf(optarg, "%02hhx%02hhx", data, data + 1);
							}
							else // all others
							{
								res = sscanf(optarg, "%02hhx%02hhx%02hhx%02hhx", data, data + 1, data + 2, data + 3);
							}
							
							if(res == (int) cbData)
							{
								bIsWrite = true;
							}
						}
						
						if(!bIsWrite)
						{
							bIsBadCli = true;
						}
					}
				
				break;
			
			default: // includes -h
				bIsBadCli = true;
				
		}
	}
	
	if(!bIsBadCli)
	{
		if(bIsBlock && (bIsRead || bIsWrite))
		{
			if(bIsRead && bIsWrite)
			{
				printf("|mode   : read then write\n");
			}
			else if(bIsRead)
			{
				printf("|mode   : read\n");
			}
			else if(bIsWrite)
			{
				printf("|mode   : write\n");
			}
			
			printf("|blk num: 0x%02hhx\n", blockNumber);
			if(bIsWrite)
			{
				printf("|data   : ");
				print_hex(data, cbData);
				printf("\n");
			}
		}
		else if(!bIsRead && !bIsWrite && !bIsBlock)
		{
			printf("|mode   : info\n");
		}
		else bIsBadCli = true;
	}
	
	if(!bIsBadCli)
	{
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
					
					if (nfc_initiator_select_passive_target(pnd, nm, NULL, 0, &nt) > 0)
					{
						stcurrent = get_info(&nt, true);
						if(stcurrent)
						{
							printf("\n");

							if(bIsBlock && (bIsRead || bIsWrite))
							{
								if(bIsRead)
								{
									get_block_at(pnd, blockNumber, NULL, 0, true);
								}
								
								if(bIsWrite)
								{
									set_block_at_confirmed(pnd, blockNumber, data, cbData, true);
								}
							}
							else if(!bIsRead && !bIsWrite && !bIsBlock)
							{
								for(i = 0; i < stcurrent->nbNormalBlock; i++)
								{
									get_block_at(pnd, i, NULL, 0, true);
								}
								display_system_info(pnd, stcurrent);
							}
						}
					}
				}
				else printf("ERROR - nfc_initiator_init: %i\n", res);
				
				nfc_close(pnd);
			}
			else printf("ERROR - nfc_open\n");
			
			nfc_exit(context);
		}
		else printf("ERROR - nfc_init\n");
	}
	else
	{
		printf(
			"Usage:\n"
			"  %s [-i]\n"
			"  %s -b N -r\n"
			"  %s -b N [-r] -w ABCD[EF01]\n  %s -h\n"
			"Options:\n"
			"  -i               (default) information mode - will try to dump the tag content and display informations\n"
			"  -b N             specify block number to operate on (tag dependent), needed for read (-r) and write (-w) modes\n"
			"  -r               read mode - will try to read block (specified with -b N parameter)\n"
			"  -w ABCD[EF01]    write mode - will try to write specicied data (2 or 4 bytes depending on tag) to block (specified with -b N parameter)\n"
			"  -h               this help\n"
			"Examples:\n"
			"  %s -i\n"
			"        Display all tag informations\n"
			"  %s -b 0x0e -r\n"
			"        Read block 0x0e (14) of the tag\n"
			"  %s -b 0x0d -w 0123abcd\n"
			"        Write block 0x0d (13) of the tag with hexadecimal value '01 23 ab cd'\n"
			"  %s -b 0x0c -r -w 0123abcd\n"
			"        Read, then write block 0x0c (12) of the tag with hexadecimal value '01 23 ab cd'\n"
			"Warnings:\n"
			"  Be careful with: system area, counters & otp, bytes order.\n"
		, argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
	}
	
	return 0;
}

bool get_block_at(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt)
{
	bool bRet = false;
	uint8_t tx[2] = {0x08, block}, rx[ST25TB_SR_BLOCK_MAX_SIZE]; // 4 is the maximum, SR176 (only 2) will fit
	int res;
	
	res = nfc_initiator_transceive_bytes(pnd, tx, sizeof(tx), rx, sizeof(rx), 0);
	if((res == 2) || (res == 4))
	{
		if(data)
		{
			if(cbData == res)
			{
				memcpy(data, rx, res);
				bRet = true;
			}
			else printf("ERROR - We got %i bytes for a %hhu buffer size?\n", res, cbData);
		}
		else bRet = true;
		
		if(bPrintIt)
		{
			printf("[0x%02hhx] ", block);
			print_hex(rx, res);
			printf("\n");
		}
	}
	else if(res > 0)
	{
		printf("ERROR - We got %i bytes?\n", res);
	}
	else printf("ERROR - nfc_initiator_transceive_bytes(get): %i\n", res);
	
	return bRet;
}

bool set_block_at(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt)
{
	bool bRet = false;
	uint8_t tx[2 + ST25TB_SR_BLOCK_MAX_SIZE] = {0x09, block}; // 4 is the maximum, SR176 (only 2) will fit
	int res;
	
	if(cbData <= ST25TB_SR_BLOCK_MAX_SIZE)
	{
		memcpy(tx + 2, data, cbData);
		
		if(bPrintIt)
		{
			printf(">0x%02hhx> ", block);
			print_hex(data, cbData);
			printf("\n");
		}
		
		res = nfc_initiator_transceive_bytes(pnd, tx, 2 + cbData, NULL, 0, 0);
		if(res == NFC_ERFTRANS) // ? :')
		{
			bRet = true;
		}
		else printf("ERROR - nfc_initiator_transceive_bytes(set): %i\n", res);
	}
	else printf("ERROR - Wanted to write %hhu bytes, but maximum is %hhu\n", cbData, ST25TB_SR_BLOCK_MAX_SIZE);
	
	return bRet;
}

bool set_block_at_confirmed(nfc_device *pnd, uint8_t block, uint8_t *data, uint8_t cbData, bool bPrintIt)
{
	bool bRet = false;
	uint8_t buffer[ST25TB_SR_BLOCK_MAX_SIZE]; // maximum size will be checked in set_block_at
	
	if(set_block_at(pnd, block, data, cbData, bPrintIt))
	{
		if(get_block_at(pnd, block, buffer, cbData, bPrintIt))
		{
			if(memcmp(data, buffer, cbData) == 0)
			{
				bRet = true;
			}
			else if(bPrintIt) 
			{
				printf("WARNING - not same value readed after write\n");
			}
		}
	}
	
	return bRet;
}

void get_info_st25tb512(uint8_t * systemArea)
{
	uint8_t b, i;
	
	b = ((*(uint32_t *) systemArea) >> 15) & 1;
	
	printf("  | ST reserved  : ");
	for(i = 0; i < 15; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n  | b15          : %hhu - %sOTP (?)\n  | OTP_Lock_Reg : ", b, b ? "not " : "");
	for(i = 16; i < 32; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n");
	for(i = 16; i < 32; i++)
	{
		if(!(((*(uint32_t *) systemArea) >> i) & 1))
		{
			printf("     block 0x%02hhx is write protected\n", ((uint8_t) (i - 16)));
		}
	}
}

void get_info_st25tb2k_4k(uint8_t * systemArea)
{
	uint8_t b, i;
	
	b = ((*(uint32_t *) systemArea) >> 15) & 1;
	
	printf("  | ST reserved  : ");
	for(i = 0; i < 15; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n  | b15          : %hhu - %sOTP (?)\n  | OTP_Lock_RegU: ", b, b ? "not " : "");
	for(i = 16; i < 24; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n  | OTP_Lock_Reg : ");
	for(i = 24; i < 32; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n");
	if(!(((*(uint32_t *) systemArea) >> 24) & 1))
	{
		printf("     blocks 0x07 and 0x08 are write protected\n");
	}
	for(i = 25; i < 32; i++)
	{
		if(!(((*(uint32_t *) systemArea) >> i) & 1))
		{
			printf("     block 0x%02hhx is write protected\n", ((uint8_t) (i - 16)));
		}
	}
}

void get_info_sr176_legacy(uint8_t * systemArea)
{
	uint8_t i;
	
	printf("  | Fixed Chip_ID: 0x%1x\n  | ST reserved  : ", systemArea[0] & 0x0f);
	for(i = 4; i < 8; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint16_t *) systemArea) >> i) & 1));
	}
	printf("\n  | OTP_Lock_Reg : ");
	for(i = 8; i < 16; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint16_t *) systemArea) >> i) & 1));
	}
	printf("\n");
	for(i = 8; i < 16; i++)
	{
		if(((*(uint16_t *) systemArea) >> i) & 1)
		{
			printf("     blocks 0x%02hhx and 0x%02hhx are write protected\n", (uint8_t) ((i - 8) * 2), (uint8_t) (((i - 8) * 2) + 1));
		}
	}
}

void get_info_sri_srt_512_legacy(uint8_t * systemArea)
{
	uint8_t b, i;
	
	b = ((*(uint32_t *) systemArea) >> 15) & 1;
	
	printf("  | Fixed Chip_ID: 0x%02hhx\n  | ST reserved  : ", systemArea[0]);
	for(i = 8; i < 15; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n  | b15          : %hhu - %sOTP (?)\n  | OTP_Lock_Reg : ", b, b ? "not " : "");
	for(i = 16; i < 32; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n");
	for(i = 16; i < 32; i++)
	{
		if(!(((*(uint32_t *) systemArea) >> i) & 1))
		{
			printf("     block 0x%02hhx is write protected\n", (uint8_t) (i - 16));
		}
	}
}

void get_info_sri2k_4k_srix4k_srix512_legacy(uint8_t * systemArea)
{
	uint8_t i;
	
	printf("  | Fixed Chip_ID: 0x%02hhx\n  | ST reserved  : ", systemArea[0]);
	for(i = 8; i < 24; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n  | OTP_Lock_Reg : ");
	for(i = 24; i < 32; i++)
	{
		printf("%hhu", (uint8_t) (((*(uint32_t *) systemArea) >> i) & 1));
	}
	printf("\n");
	if(!(((*(uint32_t *) systemArea) >> 24) & 1))
	{
		printf("     blocks 0x07 and 0x08 are write protected\n");
	}
	for(i = 25; i < 32; i++)
	{
		if(!(((*(uint32_t *) systemArea) >> i) & 1))
		{
			printf("     block 0x%02hhx is write protected\n", (uint8_t) (i - 16));
		}
	}
}

const st_data STRefs[] = {
	{0x1b, false, "ST25TB512-AC", "https://www.st.com/resource/en/datasheet/st25tb512-ac.pdf", 4, 16,  255,  get_info_st25tb512},
	{0x33, false, "ST25TB512-AT", "https://www.st.com/resource/en/datasheet/st25tb512-at.pdf", 4, 16,  255,  get_info_st25tb512},
	{0x3f, false, "ST25TB02K",    "https://www.st.com/resource/en/datasheet/st25tb02k.pdf",    4, 64,  255,  get_info_st25tb2k_4k},
	{0x1f, false, "ST25TB04K",    "https://www.st.com/resource/en/datasheet/st25tb04k.pdf",    4, 128, 255,  get_info_st25tb2k_4k},
};
const st_data STRefs_legacy[] = {
	{ 0, true, "SRI4K(s)", NULL,                                                   4, 128, 255, NULL},
	{ 2, true, "SR176",    "https://www.st.com/resource/en/datasheet/sr176.pdf",   2, 15,  15,  get_info_sr176_legacy},
	{ 3, true, "SRIX4K",   NULL,                                                   4, 128, 255, get_info_sri2k_4k_srix4k_srix512_legacy},
	{ 4, true, "SRIX512",  "https://www.st.com/resource/en/datasheet/srix512.pdf", 4, 16,  255, get_info_sri2k_4k_srix4k_srix512_legacy},
	{ 6, true, "SRI512",   "https://www.st.com/resource/en/datasheet/sri512.pdf",  4, 16,  255, get_info_sri_srt_512_legacy},
	{ 7, true, "SRI4K",    "https://www.st.com/resource/en/datasheet/sri4k.pdf",   4, 128, 255, get_info_sri2k_4k_srix4k_srix512_legacy},
	{12, true, "SRT512",   "https://www.st.com/resource/en/datasheet/srt512.pdf",  4, 16,  255, get_info_sri_srt_512_legacy},
	{15, true, "SRI2K",    "https://www.st.com/resource/en/datasheet/sri2k.pdf",   4, 64,  255, get_info_sri2k_4k_srix4k_srix512_legacy},
};

const st_data * get_info(const nfc_target *pnt, bool bPrintIt)
{
	const st_data *currentData = NULL;
	const uint8_t *p;
	uint8_t chipId, i;
	
	if(pnt->nm.nmt == NMT_ISO14443B2SR)
	{
		printf("Target  : %s (%s)\nUID     : ", str_nfc_modulation_type(pnt->nm.nmt), str_nfc_baud_rate(pnt->nm.nbr));
		print_hex(pnt->nti.nsi.abtUID, sizeof(pnt->nti.nsi.abtUID));
		printf("\n");
		
		p = pnt->nti.nsi.abtUID;
		if(p[7] == 0xd0) // ST25TB* / SR*
		{
			chipId = p[5];
			printf("Manuf   : 0x%02hhx - %s\n", p[6], (p[6] == 0x02) ? "STMicroelectronics" : "other");
			
			for(i = 0; i < (sizeof(STRefs) / sizeof(STRefs[0])); i++)
			{
				if(chipId == STRefs[i].chipId)
				{
					currentData = &STRefs[i];
					break;
				}
			}
			
			if(!currentData)
			{
				chipId >>= 2;
				for(i = 0; i < (sizeof(STRefs_legacy) / sizeof(STRefs_legacy[0])); i++)
				{
					if(chipId == STRefs_legacy[i].chipId)
					{
						currentData = &STRefs_legacy[i];
						break;
					}
				}
			}
			
			if(bPrintIt && currentData)
			{
				printf("ChipId  : 0x%02hhx - %s%s\nSerial  : 0x", currentData->chipId, currentData->szName, currentData->bIsLegacy ? " (legacy)" : "");
				if(currentData->bIsLegacy)
				{
					printf("%1hhx", (uint8_t) (p[5] & 0x03));
				}
				printf("%02hhx%02hhx%02hhx%02hhx%02hhx\n|blk sz : %hhu bits\n|nb blks: %hhu\n|sys idx: %hhu\n", p[4], p[3], p[2], p[1], p[0], (uint8_t) (currentData->blockSize * 8), currentData->nbNormalBlock, currentData->bnSystem);
			}
		}
		else printf("WARNI - Last byte of UID isn\'t 0xd0, but 0x%02hhx (not ST25TB / SR series?)\n", p[7]);
	}
	else printf("ERROR - not a NMT_ISO14443B2SR ?\n");
	
	return currentData;
}

void display_system_info(nfc_device *pnd, const st_data * stdata)
{
	uint8_t systemArea[ST25TB_SR_BLOCK_MAX_SIZE];
	
	if(get_block_at(pnd, stdata->bnSystem, systemArea, stdata->blockSize, true))
	{
		if(stdata->pfnGetInfo)
		{
			stdata->pfnGetInfo(systemArea);
		}
	}
}

void print_hex(const uint8_t *pbtData, const size_t szBytes)
{
	size_t szPos;
	for (szPos = 0; szPos < szBytes; szPos++)
	{
		printf("%02hhx ", pbtData[szPos]);
	}
}

