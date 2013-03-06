# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_LTLIBRARIES = libnfcbuses.la

libnfcbuses_la_SOURCES = 
libnfcbuses_la_CFLAGS = -I$(top_srcdir)/libnfc
libnfcbuses_la_LIBADD = 

# UART_ENABLED
libnfcbuses_la_SOURCES += uart.c uart.h
libnfcbuses_la_CFLAGS +=
libnfcbuses_la_LIBADD +=
EXTRA_DIST = uart_posix.c uart_win32.c

if LIBUSB_ENABLED
  libnfcbuses_la_SOURCES += usbbus.c usbbus.h
  libnfcbuses_la_CFLAGS += @libusb_CFLAGS@
  libnfcbuses_la_LIBADD  += @libusb_LIBS@
endif

