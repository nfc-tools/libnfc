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

#ifndef __NFC_TIMING_H__
#define __NFC_TIMING_H__

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t ms_t;

/**
 * @brief Calculates a timestamp from a unspecified start with millisecond accuracy
 * @return Time in milliseconds
 */
ms_t time_millis();

typedef ms_t timeout_t;

/**
 * @brief Initializes a timeout
 * @param to Timeout handle
 * @param millis Minimum expiration time in milliseconds
 */
void timeout_init(timeout_t * to, unsigned int millis);

/**
 * @brief Initializes a timeout which never expires
 * @param to Timeout handle
 */
void timeout_never(timeout_t * to);

/**
 * @brief Checks if the timeout has NOT expired
 * @param to Timeout handle
 * @return True if the timeout has NOT expired
 */
bool timeout_check(timeout_t * to);

#endif
