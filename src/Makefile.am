bin_PROGRAMS = anticol list mftool relay emulate

# set the include path found by configure
INCLUDES= $(all_includes)

nfcinclude_HEADERS = libnfc.h bitutils.h defines.h dev_acr122.h dev_pn531.h types.h mifaretag.h devices.h
nfcincludedir = $(includedir)/libnfc

lib_LTLIBRARIES = libnfc.la

libnfc_la_CFLAGS = @LIBUSB_CFLAGS@ @LIBPCSCLITE_CFLAGS@
libnfc_la_SOURCES = dev_pn531.c dev_acr122.c bitutils.c libnfc.c
libnfc_la_LIBADD = @LIBUSB_LIBS@ @LIBPCSCLITE_LIBS@

anticol_SOURCES = anticol.c
anticol_LDADD = libnfc.la

list_SOURCES = list.c
list_LDADD = libnfc.la

mftool_SOURCES = mftool.c
mftool_LDADD = libnfc.la

relay_SOURCES = relay.c
relay_LDADD = libnfc.la

emulate_SOURCES = emulate.c
emulate_LDADD = libnfc.la
