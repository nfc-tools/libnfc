SUBDIRS = chips buses drivers .

# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS) -DSYSCONFDIR='"$(sysconfdir)"'

lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = \
		    conf.c \
		    iso14443-subr.c \
		    mirror-subr.c \
		    nfc.c \
		    nfc-device.c \
		    nfc-emulation.c \
		    nfc-internal.c \
		    target-subr.c \
		    conf.h \
		    drivers.h \
		    iso7816.h \
		    log.h \
		    log-internal.h \
		    mirror-subr.h \
		    nfc-internal.h \
		    target-subr.h

libnfc_la_LDFLAGS = -no-undefined -version-info 5:0:0 -export-symbols-regex '^nfc_|^iso14443a_|^str_nfc_|pn53x_transceive|pn532_SAMConfiguration|pn53x_read_register|pn53x_write_register'
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

if WITH_LOG
  libnfc_la_SOURCES += log.c log-internal.c
endif

EXTRA_DIST = \
	CMakeLists.txt
