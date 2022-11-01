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
    mfcuk_finger.h

 Description:
    MFCUK fingerprinting and specific data-decoding functionality.

 License:
    GPL2, Copyright (C) 2009, Andrei Costin

 * @file mfcuk_finger.h
 * @brief MFCUK fingerprinting and specific data-decoding functionality.
*/

#ifndef _MFCUK_FINGER_H_
#define _MFCUK_FINGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mfcuk_mifare.h"

// Wrapping an ugly template into an externally pleasant name. To implement proper template later.
typedef struct _mfcuk_finger_template_ {
  mifare_classic_tag mask;
  mifare_classic_tag values;
} mfcuk_finger_template;

// Function type definition, to be used for custom decoders/comparators
typedef int (*mfcuk_finger_comparator)(mifare_classic_tag *dump, mfcuk_finger_template *tmpl, float *score);
typedef int (*mfcuk_finger_decoder)(mifare_classic_tag *dump);

// Naive implementation of a self-contained fingerprint database entry
typedef struct _mfcuk_finger_tmpl_entry_ {
  const char *tmpl_filename;
  const char *tmpl_name;
  mfcuk_finger_comparator tmpl_comparison_func;
  mfcuk_finger_decoder tmpl_decoder_func;
  mfcuk_finger_template *tmpl_data;
} mfcuk_finger_tmpl_entry;

int mfcuk_finger_default_comparator(mifare_classic_tag *dump, mfcuk_finger_template *tmpl, float *score);
int mfcuk_finger_default_decoder(mifare_classic_tag *dump);
int mfcuk_finger_skgt_decoder(mifare_classic_tag *dump);

// "Housekeeping" functions
int mfcuk_finger_load(void);
int mfcuk_finger_unload(void);

#endif
