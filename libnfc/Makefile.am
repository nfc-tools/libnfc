SUBDIRS = chips buses drivers .

# set the include path found by configure
INCLUDES = $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS =  chips.h buses.h drivers.h mirror-subr.h
lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = nfc.c iso14443-subr.c mirror-subr.c
libnfc_la_LDFLAGS = -no-undefined -version-info 1:0:0
libnfc_la_CFLAGS = @DRIVERS_CFLAGS@
libnfc_la_LIBADD = \
	$(top_builddir)/libnfc/chips/libnfcchips.la \
	$(top_builddir)/libnfc/buses/libnfcbuses.la \
	$(top_builddir)/libnfc/drivers/libnfcdrivers.la

if PCSC_ENABLED
  libnfc_la_CFLAGS += @libpcsclite_CFLAGS@ -DHAVE_PCSC
  libnfc_la_LIBADD += @libpcsclite_LIBS@
endif

if LIBUSB_ENABLED
  libnfc_la_CFLAGS += @libusb_CFLAGS@ -DHAVE_LIBUSB
  libnfc_la_LIBADD  += @libusb_LIBS@
endif

EXTRA_DIST = CMakeLists.txt
