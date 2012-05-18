# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = acr122_pcsc.h acr122_usb.h acr122s.h arygon.h pn532_uart.h pn53x_usb.h
noinst_LTLIBRARIES = libnfcdrivers.la

libnfcdrivers_la_SOURCES = 
libnfcdrivers_la_CFLAGS = @DRIVERS_CFLAGS@ -I$(top_srcdir)/libnfc -I$(top_srcdir)/libnfc/buses
libnfcdrivers_la_LIBADD = 

if DRIVER_ACR122_PCSC_ENABLED
libnfcdrivers_la_SOURCES += acr122_pcsc.c
endif

if DRIVER_ACR122_USB_ENABLED
libnfcdrivers_la_SOURCES += acr122_usb.c
endif

if DRIVER_ACR122S_ENABLED
libnfcdrivers_la_SOURCES += acr122s.c
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

