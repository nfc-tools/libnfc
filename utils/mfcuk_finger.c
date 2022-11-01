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

/*
 Package:
    MiFare Classic Universal toolKit (MFCUK)

 Package version:
    0.1

 Filename:
    mfcuk_finger.c

 Description:
    MFCUK fingerprinting and specific data-decoding functionality.

 License:
    GPL2, Copyright (C) 2009, Andrei Costin

 * @file mfcuk_finger.c
 * @brief MFCUK fingerprinting and specific data-decoding functionality.
 * @todo add proper error codes
*/

#include "mfcuk_finger.h"

mfcuk_finger_tmpl_entry mfcuk_finger_db[] = {
  { "./data/tmpls_fingerprints/mfcuk_tmpl_skgt.mfd", "Sofia SKGT", mfcuk_finger_default_comparator, mfcuk_finger_skgt_decoder, NULL },
  { "./data/tmpls_fingerprints/mfcuk_tmpl_ratb.mfd", "Bucharest RATB", mfcuk_finger_default_comparator, mfcuk_finger_default_decoder, NULL },
  { "./data/tmpls_fingerprints/mfcuk_tmpl_oyster.mfd", "London OYSTER", mfcuk_finger_default_comparator, mfcuk_finger_default_decoder, NULL },
};

int mfcuk_finger_db_entries = sizeof(mfcuk_finger_db) / sizeof(mfcuk_finger_db[0]);

int mfcuk_finger_default_decoder(mifare_classic_tag *dump)
{
  if (!dump) {
    fprintf(stderr, "ERROR: cannot decode a NULL pointer :)\n");
    return 0;
  }

  printf("UID:\t%02x%02x%02x%02x\n", dump->amb[0].mbm.abtUID[0], dump->amb[0].mbm.abtUID[1], dump->amb[0].mbm.abtUID[2], dump->amb[0].mbm.abtUID[3]);
  printf("TYPE:\t%02x\n", dump->amb[0].mbm.btSAK);

  return 1;
}

// Yes, I know C++ class inheritance would perfectly fit the decoders/comparators... Though C is more to my heart. Anyone to rewrite in C++?
int mfcuk_finger_skgt_decoder(mifare_classic_tag *dump)
{
  if (!dump) {
    fprintf(stderr, "ERROR: cannot decode a NULL pointer :)\n");
    return 0;
  }

  printf("Bulgaria/Sofia/SKGT public transport card information decoder (info credits to Andy)\n");
  mfcuk_finger_default_decoder(dump);

  printf("LAST TRAVEL DATA\n");

  // TODO: get proper information

  return 1;
}

int mfcuk_finger_default_comparator(mifare_classic_tag *dump, mfcuk_finger_template *tmpl, float *score)
{
  int max_bytes = 0;
  int i;
  int num_bytes_tomatch = 0;
  int num_bytes_matched = 0;

  if ((!dump) || (!tmpl) || (!score)) {
    return 0;
  }

  if (IS_MIFARE_CLASSIC_1K_TAG(dump)) {
    max_bytes = MIFARE_CLASSIC_BYTES_PER_BLOCK * MIFARE_CLASSIC_1K_MAX_BLOCKS;
  } else if (IS_MIFARE_CLASSIC_4K_TAG(dump)) {
    max_bytes = MIFARE_CLASSIC_BYTES_PER_BLOCK * MIFARE_CLASSIC_4K_MAX_BLOCKS;
  } else {
    return 0;
  }

  for (i = 0; i < max_bytes; i++) {
    if (((char *)(&tmpl->mask))[i] == 0x0) {
      continue;
    }

    num_bytes_tomatch++;

    if (((char *)(&tmpl->values))[i] == ((char *)dump)[i]) {
      num_bytes_matched++;
    }
  }

  if (num_bytes_tomatch == 0) {
    return 0;
  } else {
    *score = (float)(num_bytes_matched) / num_bytes_tomatch;
  }

  return 1;
}

int mfcuk_finger_load(void)
{
  int i;
  mifare_classic_tag mask;
  mifare_classic_tag values;
  FILE *fp = NULL;
  size_t result = 0;
  mfcuk_finger_template *tmpl_new = NULL;

  int template_loaded_count = 0;
  for (i = 0; i < mfcuk_finger_db_entries; i++) {
    fp = fopen(mfcuk_finger_db[i].tmpl_filename, "rb");

    if (!fp) {
      fprintf(stderr, "WARN: cannot open template file '%s'\n", mfcuk_finger_db[i].tmpl_filename);
      continue;
    }

    // If not read exactly 1 record, something is wrong
    if ((result = fread((void *)(&mask), sizeof(mask), 1, fp)) != 1) {
      fprintf(stderr, "WARN: cannot read MASK from template file '%s'\n", mfcuk_finger_db[i].tmpl_filename);
      fclose(fp);
      continue;
    }

    // If not read exactly 1 record, something is wrong
    if ((result = fread((void *)(&values), sizeof(values), 1, fp)) != 1) {
      fprintf(stderr, "WARN: cannot read VALUES template file '%s'\n", mfcuk_finger_db[i].tmpl_filename);
      fclose(fp);
      continue;
    }

    if (mfcuk_finger_db[i].tmpl_data == NULL) {
      if ((tmpl_new = (mfcuk_finger_template *) malloc(sizeof(mfcuk_finger_template))) == NULL) {
        fprintf(stderr, "WARN: cannot allocate memory to template record %d\n", i);
        fclose(fp);
        continue;
      }

      memcpy(&(tmpl_new->mask), &(mask), sizeof(mask));
      memcpy(&(tmpl_new->values), &(values), sizeof(values));

      mfcuk_finger_db[i].tmpl_data = tmpl_new;
      template_loaded_count++;
    }

    if (fp) {
      fclose(fp);
      fp = NULL;
    }
  }

  return template_loaded_count;
}

int mfcuk_finger_unload(void)
{
  int i;

  for (i = 0; i < mfcuk_finger_db_entries; i++) {
    if (mfcuk_finger_db[i].tmpl_data != NULL) {
      free(mfcuk_finger_db[i].tmpl_data);
      mfcuk_finger_db[i].tmpl_data = NULL;
    }
  }

  return 1;
}
