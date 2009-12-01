# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = arygon.h pn532_uart.h
noinst_LTLIBRARIES = libnfcdrivers.la
libnfcdrivers_la_SOURCES = arygon.c pn532_uart.c
libnfcdrivers_la_CFLAGS = -I$(top_srcdir)/src/lib -I$(top_srcdir)/src/lib/buses
libnfcdrivers_la_LIBADD = 

if PCSC_LITE_ENABLED
  noinst_HEADERS += acr122.h
  libnfcdrivers_la_CFLAGS += @LIBPCSCLITE_CFLAGS@ -DHAVE_PCSC_LITE
  libnfcdrivers_la_SOURCES += acr122.c
  libnfcdrivers_la_LIBADD += @LIBPCSCLITE_LIBS@
endif

if LIBUSB_ENABLED
  noinst_HEADERS += pn531_usb.h pn533_usb.h
  libnfcdrivers_la_CFLAGS += @LIBUSB_CFLAGS@ -DHAVE_LIBUSB
  libnfcdrivers_la_SOURCES += pn531_usb.c pn533_usb.c
  libnfcdrivers_la_LIBADD  += @LIBUSB_LIBS@
endif

