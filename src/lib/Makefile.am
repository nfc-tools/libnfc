SUBDIRS = chips buses drivers .

# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = nfc.c bitutils.c
libnfc_la_LDFLAGS = -no-undefined -version-info=0:0:0
libnfc_la_CFLAGS = 
libnfc_la_LIBADD = \
	$(top_builddir)/src/lib/chips/libnfcchips.la \
	$(top_builddir)/src/lib/buses/libnfcbuses.la \
	$(top_builddir)/src/lib/drivers/libnfcdrivers.la

if PCSC_LITE_ENABLED
  libnfc_la_CFLAGS += @LIBPCSCLITE_CFLAGS@ -DHAVE_PCSC_LITE
  libnfc_la_LIBADD += @LIBPCSCLITE_LIBS@
endif

if LIBUSB_ENABLED
  libnfc_la_CFLAGS += @LIBUSB_CFLAGS@ -DHAVE_LIBUSB
  libnfc_la_LIBADD  += @LIBUSB_LIBS@
endif

EXTRA_DIST = CMakeLists.txt chips.h buses.h drivers.h bitutils.h
