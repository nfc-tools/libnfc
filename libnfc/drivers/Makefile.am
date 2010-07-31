# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = acr122.h arygon.h pn531_usb.h pn532_uart.h pn533_usb.h pn53x_usb.h
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

if DRIVER_PN531_USB_ENABLED
libnfcdrivers_la_SOURCES += pn531_usb.c
endif

if DRIVER_PN533_USB_ENABLED
libnfcdrivers_la_SOURCES += pn533_usb.c
endif


if DRIVER_PN532_UART_ENABLED
libnfcdrivers_la_SOURCES += pn532_uart.c
endif

if PCSC_ENABLED
  libnfcdrivers_la_CFLAGS += @libpcsclite_CFLAGS@
  libnfcdrivers_la_LIBADD += @libpcsclite_LIBS@
endif

if LIBUSB_ENABLED
  libnfcdrivers_la_SOURCES += pn53x_usb.c
  libnfcdrivers_la_CFLAGS += @libusb_CFLAGS@
  libnfcdrivers_la_LIBADD  += @libusb_LIBS@
endif

