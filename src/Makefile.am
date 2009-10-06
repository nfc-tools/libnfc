SUBDIRS = . examples

# set the include path found by configure
INCLUDES= $(all_includes)

nfc_devices_headers = dev_arygon.h dev_pn532_uart.h
nfcinclude_HEADERS = libnfc.h bitutils.h defines.h types.h rs232.h devices.h messages.h
nfcinclude_HEADERS += $(nfc_devices_headers)
nfcincludedir = $(includedir)/libnfc

lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = bitutils.c libnfc.c rs232.c dev_arygon.c dev_pn532_uart.c
libnfc_la_CFLAGS = 
libnfc_la_LIBADD = 

if PCSC_LITE_ENABLED
  nfcinclude_HEADERS += dev_acr122.h
  libnfc_la_CFLAGS += @LIBPCSCLITE_CFLAGS@ -DHAVE_PCSC_LITE
  libnfc_la_SOURCES += dev_acr122.c
  libnfc_la_LIBADD += @LIBPCSCLITE_LIBS@
endif

if LIBUSB_ENABLED
  nfcinclude_HEADERS += dev_pn531.h dev_pn533.h
  libnfc_la_CFLAGS += @LIBUSB_CFLAGS@ -DHAVE_LIBUSB
  libnfc_la_SOURCES += dev_pn531.c dev_pn533.c
  libnfc_la_LIBADD  += @LIBUSB_LIBS@
endif

