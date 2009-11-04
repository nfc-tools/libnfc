SUBDIRS = buses drivers .

# set the include path found by configure
INCLUDES= $(all_includes)

nfcinclude_HEADERS = nfc.h defines.h types.h buses.h drivers.h messages.h bitutils.h
nfcincludedir = $(includedir)/nfc

lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = nfc.c bitutils.c
libnfc_la_LDFLAGS = -no-undefined -version-info=0:0:0
libnfc_la_CFLAGS = 
libnfc_la_LIBADD = \
	$(top_builddir)/src/lib/buses/libnfcbuses.la \
	$(top_builddir)/src/lib/drivers/libnfcdrivers.la

DISTCLEANFILES = Makefile.in
