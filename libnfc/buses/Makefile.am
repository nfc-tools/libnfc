# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_LTLIBRARIES = libnfcbuses.la

libnfcbuses_la_SOURCES = empty.c
libnfcbuses_la_CFLAGS = -I$(top_srcdir)/libnfc
libnfcbuses_la_LIBADD = 
EXTRA_DIST =

if SPI_ENABLED
libnfcbuses_la_SOURCES += spi.c spi.h
libnfcbuses_la_CFLAGS +=
libnfcbuses_la_LIBADD +=
endif
EXTRA_DIST += spi.c spi.h

if UART_ENABLED
  libnfcbuses_la_SOURCES += uart.c uart.h
  libnfcbuses_la_CFLAGS +=
  libnfcbuses_la_LIBADD +=
endif
EXTRA_DIST += uart.c uart.h

if LIBUSB_ENABLED
  libnfcbuses_la_SOURCES += usbbus.c usbbus.h
  libnfcbuses_la_CFLAGS += @libusb_CFLAGS@
  libnfcbuses_la_LIBADD  += @libusb_LIBS@
endif
EXTRA_DIST += usbbus.c usbbus.h

if I2C_ENABLED
  libnfcbuses_la_SOURCES += i2c.c i2c.h
  libnfcbuses_la_CFLAGS +=
  libnfcbuses_la_LIBADD +=
endif
EXTRA_DIST += i2c.c i2c.h
