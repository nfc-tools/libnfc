#ifndef _ERR_H_
#define _ERR_H_

#include <stdlib.h>

#define warnx(...) do { \
    fprintf (stderr, __VA_ARGS__); \
    fprintf (stderr, "\n"); \
  } while (0)

#define errx(code, ...) do { \
    fprintf (stderr, __VA_ARGS__); \
    fprintf (stderr, "\n"); \
    exit (code); \
  } while (0)

#define err errx

#endif /* !_ERR_H_ */
