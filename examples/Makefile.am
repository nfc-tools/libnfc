bin_PROGRAMS = nfc-anticol nfc-list nfc-mfclassic nfc-mfultralight nfc-relay nfc-emulate nfcip-target nfcip-initiator nfc-poll

# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

nfcinclude_HEADERS = mifaretag.h mifareultag.h
nfcincludedir = $(includedir)/nfc

AM_CFLAGS = -I$(top_srcdir)/libnfc

nfc_poll_SOURCES = nfc-poll.c
nfc_poll_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_anticol_SOURCES = nfc-anticol.c
nfc_anticol_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_list_SOURCES = nfc-list.c
nfc_list_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_mfultralight_SOURCES = nfc-mfultralight.c mifareultag.h
nfc_mfultralight_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_mfclassic_SOURCES = nfc-mfclassic.c mifaretag.h
nfc_mfclassic_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_relay_SOURCES = nfc-relay.c
nfc_relay_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_emulate_SOURCES = nfc-emulate.c
nfc_emulate_LDADD = $(top_builddir)/libnfc/libnfc.la

nfcip_target_SOURCES = nfcip-target.c
nfcip_target_LDADD = $(top_builddir)/libnfc/libnfc.la

nfcip_initiator_SOURCES = nfcip-initiator.c
nfcip_initiator_LDADD = $(top_builddir)/libnfc/libnfc.la

dist_man_MANS = nfc-anticol.1 nfc-emulate.1 nfc-list.1 nfc-mfclassic.1 nfc-mfultralight.1 nfc-relay.1

EXTRA_DIST = CMakeLists.txt
