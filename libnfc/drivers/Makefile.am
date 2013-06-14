# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_LTLIBRARIES = libnfcdrivers.la

libnfcdrivers_la_SOURCES =
libnfcdrivers_la_CFLAGS = @DRIVERS_CFLAGS@ -I$(top_srcdir)/libnfc -I$(top_srcdir)/libnfc/buses
libnfcdrivers_la_LIBADD =

if DRIVER_ACR122_PCSC_ENABLED
libnfcdrivers_la_SOURCES += acr122_pcsc.c acr122_pcsc.h
endif

if DRIVER_ACR122_USB_ENABLED
libnfcdrivers_la_SOURCES += acr122_usb.c acr122_usb.h
endif

if DRIVER_ACR122S_ENABLED
libnfcdrivers_la_SOURCES += acr122s.c acr122s.h
endif

if DRIVER_ARYGON_ENABLED
libnfcdrivers_la_SOURCES += arygon.c arygon.h
endif

if DRIVER_PN53X_USB_ENABLED
libnfcdrivers_la_SOURCES += pn53x_usb.c pn53x_usb.h
endif

if DRIVER_PN532_UART_ENABLED
libnfcdrivers_la_SOURCES += pn532_uart.c pn532_uart.h
endif

if DRIVER_PN532_SPI_ENABLED
libnfcdrivers_la_SOURCES += pn532_spi.c pn532_spi.h
endif

if DRIVER_PN532_I2C_ENABLED
libnfcdrivers_la_SOURCES += pn532_i2c.c pn532_i2c.h
endif

if PCSC_ENABLED
  libnfcdrivers_la_CFLAGS += @libpcsclite_CFLAGS@
  libnfcdrivers_la_LIBADD += @libpcsclite_LIBS@
endif

if LIBUSB_ENABLED
  libnfcdrivers_la_CFLAGS += @libusb_CFLAGS@
  libnfcdrivers_la_LIBADD  += @libusb_LIBS@
endif

