#ifndef _ERR_H_
#define _ERR_H_

#include <stdlib.h>

#define warnx(...) fprintf (stderr, __VA_ARGS__)
#define errx(code, ...) do { \
	fprintf (stderr, __VA_ARGS__); \
	exit (code); \
} while (0)

#endif /* !_ERR_H_ */
