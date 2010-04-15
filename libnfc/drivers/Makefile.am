# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = acr122.h arygon.h pn531_usb.h pn532_uart.h pn533_usb.h pn53x_usb.h
noinst_LTLIBRARIES = libnfcdrivers.la
libnfcdrivers_la_SOURCES = acr122.c arygon.c pn531_usb.c pn532_uart.c pn533_usb.c pn53x_usb.c
libnfcdrivers_la_CFLAGS = @DRIVERS_CFLAGS@ -I$(top_srcdir)/libnfc -I$(top_srcdir)/libnfc/buses
libnfcdrivers_la_LIBADD = 

if PCSC_ENABLED
  libnfcdrivers_la_CFLAGS += @libpcsclite_CFLAGS@
  libnfcdrivers_la_LIBADD += @libpcsclite_LIBS@
endif

if LIBUSB_ENABLED
  libnfcdrivers_la_CFLAGS += @libusb_CFLAGS@
  libnfcdrivers_la_LIBADD  += @libusb_LIBS@
endif

