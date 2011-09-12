# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = acr122.h arygon.h pn532_uart.h pn53x_usb.h
noinst_LTLIBRARIES = libnfcdrivers.la

libnfcdrivers_la_SOURCES = 
libnfcdrivers_la_CFLAGS = @DRIVERS_CFLAGS@ -I$(top_srcdir)/libnfc -I$(top_srcdir)/libnfc/buses
libnfcdrivers_la_LIBADD = 

if DRIVER_ACR122_ENABLED
libnfcdrivers_la_SOURCES += acr122.c
endif

if DRIVER_ARYGON_ENABLED
libnfcdrivers_la_SOURCES += arygon.c
endif

if DRIVER_PN53X_USB_ENABLED
libnfcdrivers_la_SOURCES += pn53x_usb.c
endif

if DRIVER_PN532_UART_ENABLED
libnfcdrivers_la_SOURCES += pn532_uart.c
endif

if PCSC_ENABLED
  libnfcdrivers_la_CFLAGS += @libpcsclite_CFLAGS@
  libnfcdrivers_la_LIBADD += @libpcsclite_LIBS@
endif

if LIBUSB_ENABLED
  libnfcdrivers_la_CFLAGS += @libusb_CFLAGS@
  libnfcdrivers_la_LIBADD  += @libusb_LIBS@
endif

if HAS_LOG4C
  libnfcdrivers_la_CFLAGS += @log4c_CFLAGS@
  libnfcdrivers_la_LIBADD += @log4c_LIBS@
endif

