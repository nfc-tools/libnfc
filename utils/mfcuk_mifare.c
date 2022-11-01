/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Package version:
    0.1

 Filename:
    mfcuk_mifare.c

 Description:
    MFCUK defines and function implementation file extending
    mainly libnfc's "mifare.h" interface/functionality.

 Contact, bug-reports:
    http://andreicostin.com/
    mailto:zveriu@gmail.com

 License:
    GPL2 (see below), Copyright (C) 2009, Andrei Costin

 * @file mfcuk_mifare.c
 * @brief
*/

/*
 VERSION HISTORY
--------------------------------------------------------------------------------
| Number     : 0.1
| dd/mm/yyyy : 23/11/2009
| Author     : zveriu@gmail.com, http://andreicostin.com
| Description: Moved bulk of defines and functions from "mfcuk_keyrecovery_darkside.c"
--------------------------------------------------------------------------------
*/

/*
 LICENSE

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mfcuk_mifare.h"

// Default keys used as a *BIG* mistake in many applications - especially System Integrators should pay attention!
uint8_t mfcuk_default_keys[][MIFARE_CLASSIC_KEY_BYTELENGTH] = {
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Place-holder for current key to verify
  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
  {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd},
  {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a},
  {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7},
  {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
};

int mfcuk_default_keys_num = sizeof(mfcuk_default_keys) / sizeof(mfcuk_default_keys[0]);

bool is_valid_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (IS_MIFARE_CLASSIC_1K(bTagType) && (uiBlock < MIFARE_CLASSIC_1K_MAX_BLOCKS)) {
    return true;
  }

  if (IS_MIFARE_CLASSIC_4K(bTagType) && (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS)) {
    return true;
  }

  return false;
}

bool is_valid_sector(uint8_t bTagType, uint32_t uiSector)
{
  if (IS_MIFARE_CLASSIC_1K(bTagType) && (uiSector < MIFARE_CLASSIC_1K_MAX_SECTORS)) {
    return true;
  }

  if (IS_MIFARE_CLASSIC_4K(bTagType) && (uiSector < MIFARE_CLASSIC_4K_MAX_SECTORS)) {
    return true;
  }

  return false;
}

bool is_first_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (!is_valid_block(bTagType, uiBlock)) {
    return false;
  }

  // Test if we are in the small or big sectors
  if (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS1) {
    // For Mifare Classic 1K, it will enter always here
    return ((uiBlock) % (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) == 0);
  } else {
    // This branch will enter only for Mifare Classic 4K big sectors
    return ((uiBlock) % (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) == 0);
  }

  // Should not reach here, but... never know
  return false;
}

bool is_trailer_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (!is_valid_block(bTagType, uiBlock)) {
    return false;
  }

  // Test if we are in the small or big sectors
  if (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS1) {
    // For Mifare Classic 1K, it will enter always here
    return ((uiBlock + 1) % (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) == 0);
  } else {
    // This branch will enter only for Mifare Classic 4K big sectors
    return ((uiBlock + 1) % (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) == 0);
  }

  // Should not reach here, but... never know
  return false;
}

uint32_t get_first_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (!is_valid_block(bTagType, uiBlock)) {
    return MIFARE_CLASSIC_INVALID_BLOCK;
  }

  // Test if we are in the small or big sectors
  if (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS1) {
    // Integer divide, then integer multiply
    return (uiBlock / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1;
  } else {
    uint32_t tmp = uiBlock - MIFARE_CLASSIC_4K_MAX_BLOCKS1;
    return MIFARE_CLASSIC_4K_MAX_BLOCKS1 + (tmp / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2;
  }

  // Should not reach here, but... never know
  return MIFARE_CLASSIC_INVALID_BLOCK;
}

uint32_t get_trailer_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (!is_valid_block(bTagType, uiBlock)) {
    return MIFARE_CLASSIC_INVALID_BLOCK;
  }

  // Test if we are in the small or big sectors
  if (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS1) {
    // Integer divide, then integer multiply
    return (uiBlock / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1 + (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1 - 1);
  } else {
    uint32_t tmp = uiBlock - MIFARE_CLASSIC_4K_MAX_BLOCKS1;
    return MIFARE_CLASSIC_4K_MAX_BLOCKS1 + (tmp / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2 + (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2 - 1);
  }

  // Should not reach here, but... never know
  return MIFARE_CLASSIC_INVALID_BLOCK;
}

bool is_big_sector(uint8_t bTagType, uint32_t uiSector)
{
  if (!is_valid_sector(bTagType, uiSector)) {
    return false;
  }

  if (uiSector >= MIFARE_CLASSIC_4K_MAX_SECTORS1) {
    return true;
  }

  return false;
}

uint32_t get_first_block_for_sector(uint8_t bTagType, uint32_t uiSector)
{
  if (!is_valid_sector(bTagType, uiSector)) {
    return MIFARE_CLASSIC_INVALID_BLOCK;
  }

  if (uiSector < MIFARE_CLASSIC_4K_MAX_SECTORS1) {
    // For Mifare Classic 1K, it will enter always here
    return (uiSector * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1);
  } else {
    // For Mifare Classic 4K big sectors it will enter always here
    uint32_t tmp = uiSector - MIFARE_CLASSIC_4K_MAX_SECTORS1;
    return MIFARE_CLASSIC_4K_MAX_BLOCKS1 + (tmp * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2);
  }

  // Should not reach here, but... never know
  return MIFARE_CLASSIC_INVALID_BLOCK;
}

uint32_t get_trailer_block_for_sector(uint8_t bTagType, uint32_t uiSector)
{
  if (!is_valid_sector(bTagType, uiSector)) {
    return MIFARE_CLASSIC_INVALID_BLOCK;
  }

  if (uiSector < MIFARE_CLASSIC_4K_MAX_SECTORS1) {
    // For Mifare Classic 1K, it will enter always here
    return (uiSector * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1) + (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1 - 1);
  } else {
    // For Mifare Classic 4K big sectors it will enter always here
    uint32_t tmp = uiSector - MIFARE_CLASSIC_4K_MAX_SECTORS1;
    return MIFARE_CLASSIC_4K_MAX_BLOCKS1 + (tmp * MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2) + (MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2 - 1);
  }

  // Should not reach here, but... never know
  return MIFARE_CLASSIC_INVALID_BLOCK;
}

uint32_t get_sector_for_block(uint8_t bTagType, uint32_t uiBlock)
{
  if (!is_valid_block(bTagType, uiBlock)) {
    return MIFARE_CLASSIC_INVALID_BLOCK;
  }

  // Test if we are in the small or big sectors
  if (uiBlock < MIFARE_CLASSIC_4K_MAX_BLOCKS1) {
    // For Mifare Classic 1K, it will enter always here
    return (uiBlock / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR1);
  } else {
    uint32_t tmp = uiBlock - MIFARE_CLASSIC_4K_MAX_BLOCKS1;
    return MIFARE_CLASSIC_4K_MAX_SECTORS1 + (tmp / MIFARE_CLASSIC_4K_BLOCKS_PER_SECTOR2);
  }

  // Should not reach here, but... never know
  return MIFARE_CLASSIC_INVALID_BLOCK;
}

// Test case function for checking correct functionality of the block/sector is_ ang get_ functions
void test_mifare_classic_blocks_sectors_functions(uint8_t bTagType)
{
  uint32_t i;
  uint32_t max_blocks, max_sectors;

  if (IS_MIFARE_CLASSIC_1K(bTagType)) {
    printf("\nMIFARE CLASSIC 1K\n");
    max_blocks = MIFARE_CLASSIC_1K_MAX_BLOCKS;
    max_sectors = MIFARE_CLASSIC_1K_MAX_SECTORS;
  } else if (IS_MIFARE_CLASSIC_4K(bTagType)) {
    printf("\nMIFARE CLASSIC 4K\n");
    max_blocks = MIFARE_CLASSIC_4K_MAX_BLOCKS;
    max_sectors = MIFARE_CLASSIC_4K_MAX_SECTORS;
  } else {
    return;
  }

  // Include one invalid block, that is why we add +1
  for (i = 0; i < max_blocks + 1; i++) {
    printf("BLOCK %d\n", i);
    printf("\t is_valid_block: %c\n", (is_valid_block(bTagType, i) ? 'Y' : 'N'));
    printf("\t is_first_block: %c\n", (is_first_block(bTagType, i) ? 'Y' : 'N'));
    printf("\t is_trailer_block: %c\n", (is_trailer_block(bTagType, i) ? 'Y' : 'N'));
    printf("\t get_first_block: %d\n", get_first_block(bTagType, i));
    printf("\t get_trailer_block: %d\n", get_trailer_block(bTagType, i));
    printf("\t get_sector_for_block: %d\n", get_sector_for_block(bTagType, i));
  }

  // Include one invalid sector, that is why we add +1
  for (i = 0; i < max_sectors + 1; i++) {
    printf("SECTOR %d\n", i);
    printf("\t is_valid_sector: %c\n", (is_valid_sector(bTagType, i) ? 'Y' : 'N'));
    printf("\t is_big_sector: %c\n", (is_big_sector(bTagType, i) ? 'Y' : 'N'));
    printf("\t get_first_block_for_sector: %d\n", get_first_block_for_sector(bTagType, i));
    printf("\t get_trailer_block_for_sector: %d\n", get_trailer_block_for_sector(bTagType, i));
  }

}

bool mfcuk_save_tag_dump(const char *filename, mifare_classic_tag *tag)
{
  FILE *fp;
  size_t result;

  fp = fopen(filename, "wb");
  if (!fp) {
    return false;
  }

  // Expect to write 1 record
  result = fwrite((void *) tag, sizeof(*tag), 1, fp);

  // If not written exactly 1 record, something is wrong
  if (result != 1) {
    fclose(fp);
    return false;
  }

  fclose(fp);
  return true;
}

bool mfcuk_save_tag_dump_ext(const char *filename, mifare_classic_tag_ext *tag_ext)
{
  FILE *fp;
  size_t result;

  fp = fopen(filename, "wb");
  if (!fp) {
    return false;
  }

  // Expect to write 1 record
  result = fwrite((void *) tag_ext, sizeof(*tag_ext), 1, fp);

  // If not written exactly 1 record, something is wrong
  if (result != 1) {
    fclose(fp);
    return false;
  }

  fclose(fp);
  return true;
}

bool mfcuk_load_tag_dump(const char *filename, mifare_classic_tag *tag)
{
  FILE *fp;
  size_t result;

  fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  // Expect to read 1 record
  result = fread((void *) tag, sizeof(*tag), 1, fp);

  // If not read exactly 1 record, something is wrong
  if (result != 1) {
    fclose(fp);
    return false;
  }

  fclose(fp);
  return true;
}

bool mfcuk_load_tag_dump_ext(const char *filename, mifare_classic_tag_ext *tag_ext)
{
  FILE *fp;
  size_t result;

  fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  // Expect to read 1 record
  result = fread((void *) tag_ext, sizeof(*tag_ext), 1, fp);

  // If not read exactly 1 record, something is wrong
  if (result != sizeof(*tag_ext)) {
    fclose(fp);
    return false;
  }

  fclose(fp);
  return true;
}

void print_mifare_classic_tag_keys(const char *title, mifare_classic_tag *tag)
{
  uint32_t i, max_blocks, trailer_block;
  uint8_t bTagType;
  mifare_classic_block_trailer *ptr_trailer = NULL;

  if (!tag) {
    return;
  }

  bTagType = tag->amb->mbm.btSAK;

  if (!IS_MIFARE_CLASSIC_1K(bTagType) && !IS_MIFARE_CLASSIC_4K(bTagType)) {
    return;
  }

  printf("%s - UID %02x %02x %02x %02x - TYPE 0x%02x (%s)\n",
         title, tag->amb->mbm.abtUID[0], tag->amb->mbm.abtUID[1], tag->amb->mbm.abtUID[2], tag->amb->mbm.abtUID[3], bTagType,
         (IS_MIFARE_CLASSIC_1K(bTagType) ? (MIFARE_CLASSIC_1K_NAME) : (IS_MIFARE_CLASSIC_4K(bTagType) ? (MIFARE_CLASSIC_4K_NAME) : (MIFARE_CLASSIC_UNKN_NAME)))
        );
  printf("-------------------------------------------------------\n");
  printf("Sector\t|    Key A\t|    AC bits\t|    Key B\n");
  printf("-------------------------------------------------------\n");

  if (IS_MIFARE_CLASSIC_1K(tag->amb->mbm.btSAK)) {
    max_blocks = MIFARE_CLASSIC_1K_MAX_BLOCKS;
  } else {
    max_blocks = MIFARE_CLASSIC_4K_MAX_BLOCKS;
  }

  for (i = 0; i < max_blocks; i++) {
    trailer_block = get_trailer_block(bTagType, i);

    if (!is_valid_block(bTagType, trailer_block)) {
      break;
    }

    ptr_trailer = (mifare_classic_block_trailer *)((char *)tag + (trailer_block * MIFARE_CLASSIC_BYTES_PER_BLOCK));

    printf("%d\t|  %02x%02x%02x%02x%02x%02x\t|  %02x%02x%02x%02x\t|  %02x%02x%02x%02x%02x%02x\n",
           get_sector_for_block(bTagType, trailer_block),
           ptr_trailer->abtKeyA[0], ptr_trailer->abtKeyA[1], ptr_trailer->abtKeyA[2],
           ptr_trailer->abtKeyA[3], ptr_trailer->abtKeyA[4], ptr_trailer->abtKeyA[5],
           ptr_trailer->abtAccessBits[0], ptr_trailer->abtAccessBits[1], ptr_trailer->abtAccessBits[2], ptr_trailer->abtAccessBits[3],
           ptr_trailer->abtKeyB[0], ptr_trailer->abtKeyB[1], ptr_trailer->abtKeyB[2],
           ptr_trailer->abtKeyB[3], ptr_trailer->abtKeyB[4], ptr_trailer->abtKeyB[5]
          );

    // Go beyond current trailer block, i.e. go to next sector
    i = trailer_block;
  }

  printf("\n");

  return;
}

bool mfcuk_key_uint64_to_arr(const uint64_t *ui64Key, uint8_t *arr6Key)
{
  int i;

  if (!ui64Key || !arr6Key) {
    return false;
  }

  for (i = 0; i < MIFARE_CLASSIC_KEY_BYTELENGTH; i++) {
    arr6Key[i] = (uint8_t)(((*ui64Key) >> 8 * (MIFARE_CLASSIC_KEY_BYTELENGTH - i - 1)) & 0xFF);
  }

  return true;
}

bool mfcuk_key_arr_to_uint64(const uint8_t *arr6Key, uint64_t *ui64Key)
{
  uint64_t key = 0;
  int i;

  if (!ui64Key || !arr6Key) {
    return false;
  }

  for (i = 0; i < MIFARE_CLASSIC_KEY_BYTELENGTH; i++, key <<= 8) {
    key |= arr6Key[i];
  }
  key >>= 8;

  *ui64Key = key;

  return true;
}
