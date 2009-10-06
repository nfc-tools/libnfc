bin_PROGRAMS = nfc-anticol nfc-list nfc-mftool nfc-mfultool nfc-relay nfc-emulate nfcip-target nfcip-initiator

# set the include path found by configure
INCLUDES= $(all_includes)

AM_CFLAGS = -I$(top_srcdir)/src

nfc_anticol_SOURCES = nfc-anticol.c
nfc_anticol_LDADD = $(top_builddir)/src/libnfc.la

nfc_list_SOURCES = nfc-list.c
nfc_list_LDADD = $(top_builddir)/src/libnfc.la

nfc_mfultool_SOURCES = nfc-mfultool.c mifareultag.h
nfc_mfultool_LDADD = $(top_builddir)/src/libnfc.la

nfc_mftool_SOURCES = nfc-mftool.c mifaretag.h
nfc_mftool_LDADD = $(top_builddir)/src/libnfc.la

nfc_relay_SOURCES = nfc-relay.c
nfc_relay_LDADD = $(top_builddir)/src/libnfc.la

nfc_emulate_SOURCES = nfc-emulate.c
nfc_emulate_LDADD = $(top_builddir)/src/libnfc.la

nfcip_target_SOURCES = nfcip-target.c
nfcip_target_LDADD = $(top_builddir)/src/libnfc.la

nfcip_initiator_SOURCES = nfcip-initiator.c
nfcip_initiator_LDADD = $(top_builddir)/src/libnfc.la

dist_man_MANS = nfc-anticol.1 nfc-emulate.1 nfc-list.1 nfc-mftool.1 nfc-mfultool.1 nfc-relay.1
#dist_man_MANS = $(man_MANS)
#EXTRA_DIST = $(man_MANS)

