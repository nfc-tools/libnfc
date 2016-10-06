
#ifndef __NFC_DRIVER_LIBNFC_NCI_H__
#define __NFC_DRIVER_LIBNFC_NCI_H__

#include "linux_nfc_api.h"

#define LOG_CATEGORY "libnfc.driver.pn71xx"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

static bool IsTechnology(nfc_tag_info_t *TagInfo, nfc_modulation_type nmt)
{
	switch (nmt) {
	case NMT_ISO14443A:
		if (TagInfo->technology == TARGET_TYPE_ISO14443_4
				|| TagInfo->technology == TARGET_TYPE_ISO14443_3A
				|| TagInfo->technology == TARGET_TYPE_MIFARE_CLASSIC
				|| TagInfo->technology == TARGET_TYPE_MIFARE_UL)
			return true;
		break;

	case NMT_ISO14443B:
	case NMT_ISO14443BI:
	case NMT_ISO14443B2SR:
	case NMT_ISO14443B2CT:
		if (TagInfo->technology == TARGET_TYPE_ISO14443_3B) 
			return true;
		break;

	case NMT_FELICA:
		if (TagInfo->technology == TARGET_TYPE_FELICA)
			return true;
		break;

	case NMT_JEWEL:
		if (TagInfo->technology == TARGET_TYPE_ISO14443_3A)
			return true;
		break;

	default:
		return false;
	}
	return false;
}

static void BufferPrintBytes(char* buffer, unsigned int buflen, unsigned char* data, unsigned int datalen)
{
	int cx = 0;
	for(int i = 0x00; i < datalen; i++)	{
		cx += snprintf(buffer + cx, buflen - cx, "%02X ", data[i]);
	}
}

static void PrintTagInfo (nfc_tag_info_t *TagInfo)
{
	switch (TagInfo->technology)
	{
		case TARGET_TYPE_UNKNOWN:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type Unknown'");
		} break;
		case TARGET_TYPE_ISO14443_3A:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A'");
		} break;
		case TARGET_TYPE_ISO14443_3B:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type 4B'");
		} break;
		case TARGET_TYPE_ISO14443_4:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type 4A'");
		} break;
		case TARGET_TYPE_FELICA:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type F'");
		} break;
		case TARGET_TYPE_ISO15693:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type V'");
		} break;
		case TARGET_TYPE_NDEF:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type NDEF'");
		} break;
		case TARGET_TYPE_NDEF_FORMATABLE:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type Formatable'");
		} break;
		case TARGET_TYPE_MIFARE_CLASSIC:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Mifare Classic'");
		} break;
		case TARGET_TYPE_MIFARE_UL:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Mifare Ul'");
		} break;
		case TARGET_TYPE_KOVIO_BARCODE:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A - Kovio Barcode'");
		} break;
		case TARGET_TYPE_ISO14443_3A_3B:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type A/B'");
		} break;
		default:
		{
			log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "'Type %d (Unknown or not supported)'\n", TagInfo->technology);
		} break;
	}
	/*32 is max UID len (Kovio tags)*/
	if((0x00 != TagInfo->uid_length) && (32 >= TagInfo->uid_length))
	{		
		char buffer [100];
		int cx = 0;

		if(4 == TagInfo->uid_length || 7 == TagInfo->uid_length || 10 == TagInfo->uid_length)
		{
			cx += snprintf(buffer + cx, sizeof(buffer) - cx, "NFCID1 :    \t'");
		}
		else if(8 == TagInfo->uid_length)
		{
			cx += snprintf(buffer + cx, sizeof(buffer) - cx, "NFCID2 :    \t'");
		}
		else
		{
			cx += snprintf(buffer + cx, sizeof(buffer) - cx, "UID :    \t'");
		}

		BufferPrintBytes(buffer + cx, sizeof(buffer) - cx, (unsigned char*) TagInfo->uid, TagInfo->uid_length);
		log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s'", buffer);
	}
}

#endif 
