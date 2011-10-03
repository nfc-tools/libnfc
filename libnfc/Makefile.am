SUBDIRS = chips buses drivers .

# set the include path found by configure
INCLUDES = $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = \
		 drivers.h \
		 iso7816.h \
		 log.h \
		 mirror-subr.h \
		 nfc-internal.h
lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = \
		    iso14443-subr.c \
		    mirror-subr.c \
		    nfc.c \
		    nfc-device.c \
		    nfc-emulation.c \
		    nfc-internal.c

libnfc_la_LDFLAGS = -no-undefined -version-info 2:0:0
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

if HAS_LOG4C
  libnfc_la_CFLAGS += @log4c_CFLAGS@
  libnfc_la_LIBADD += @log4c_LIBS@

  libnfc_la_SOURCES += log.c
endif

EXTRA_DIST = CMakeLists.txt
