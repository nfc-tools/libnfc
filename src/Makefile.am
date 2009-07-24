enable_pcsc_lite = @enable_pcsc_lite@

bin_PROGRAMS = nfc-anticol nfc-list nfc-mftool nfc-mfultool nfc-relay nfc-emulate

# set the include path found by configure
INCLUDES= $(all_includes)

nfcinclude_HEADERS = libnfc.h bitutils.h defines.h types.h mifaretag.h mifareultag.h devices.h rs232.h dev_arygon.h dev_pn531.h dev_pn533.h
nfcincludedir = $(includedir)/libnfc

lib_LTLIBRARIES = libnfc.la
libnfc_la_SOURCES = bitutils.c libnfc.c rs232.c dev_arygon.c dev_pn531.c dev_pn533.c
libnfc_la_CFLAGS = @LIBUSB_CFLAGS@
libnfc_la_LIBADD = @LIBUSB_LIBS@

if PCSC_LITE_ENABLED
  nfcinclude_HEADERS += dev_acr122.h
  libnfc_la_CFLAGS += @LIBPCSCLITE_CFLAGS@ -DHAVE_PCSC_LITE
  libnfc_la_SOURCES += dev_acr122.c
  libnfc_la_LIBADD += @LIBPCSCLITE_LIBS@
endif

nfc_anticol_SOURCES = anticol.c
nfc_anticol_LDADD = libnfc.la

nfc_list_SOURCES = list.c
nfc_list_LDADD = libnfc.la

nfc_mfultool_SOURCES = mfultool.c
nfc_mfultool_LDADD = libnfc.la

nfc_mftool_SOURCES = mftool.c
nfc_mftool_LDADD = libnfc.la

nfc_relay_SOURCES = relay.c
nfc_relay_LDADD = libnfc.la

nfc_emulate_SOURCES = emulate.c
nfc_emulate_LDADD = libnfc.la

dist_man_MANS = nfc-anticol.1 nfc-emulate.1 nfc-list.1 nfc-mftool.1 nfc-relay.1
#dist_man_MANS = $(man_MANS)
#EXTRA_DIST = $(man_MANS)

