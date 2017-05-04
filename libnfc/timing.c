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
 
#include "timing.h"
#include <assert.h>

#define MILLIS_PER_SEC 1000
#define MICROS_PER_SEC 1000000
#define NANOS_PER_SEC 1000000000

#define MAGIC_EXPIRED	((uint64_t) -1)
#define MAGIC_NEVER	((uint64_t) -2)

// Use Windows' API directly if Win32 for highest possible resolution
#if defined(CYGWIN) || defined(_WIN32)
#	include <windows.h>

	static uint64_t timer_hz = 0;

	ms_t time_millis() {
		if (!timer_hz) {
			// Windows API states that the frequency is fixed at boot and therefore can be cached safely
			assert(QueryPerformanceFrequency((LARGE_INTEGER *) &timer_hz));
		}

		uint64_t ticks;
		assert(QueryPerformanceCounter((LARGE_INTEGER *) &ticks));
		return MILLIS_PER_SEC * ticks / timer_hz;
	}

// If not Windows use POSIX methods
#else
#	include <time.h>

	ms_t time_millis() {
		struct timespec ts;

		// CLOCK_MONOTONIC_RAW isn't affected by NTP updates and therefore really monotonic, but it's Linux-specific
#		ifdef CLOCK_MONOTONIC_RAW
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#		else
			clock_gettime(CLOCK_MONOTONIC, &ts);
#		endif

		return (ms_t) ts.tv_sec * MILLIS_PER_SEC + ts.tv_nsec / (NANOS_PER_SEC / MILLIS_PER_SEC);
	}
#endif

void timeout_init(timeout_t * to, unsigned int millis) {
	*to = time_millis() + millis;
}

void timeout_never(timeout_t * to) {
	*to = MAGIC_NEVER;
}

bool timeout_check(timeout_t * to) {
	switch (*to) {
		case MAGIC_EXPIRED:
			return false;
		case MAGIC_NEVER:
			return true;
	}

	ms_t now = time_millis();
	if (now >= *to) {
		// Mark as expired and fail in next check
		*to = MAGIC_EXPIRED;
	}

	return true;
}
