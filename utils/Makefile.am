bin_PROGRAMS = \
		nfc-emulate-forum-tag4 \
		nfc-list \
		nfc-mfclassic \
		nfc-mfultralight \
		nfc-probe \
		nfc-read-forum-tag3 \
		nfc-relay-picc

# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = mifare.h nfc-utils.h
noinst_LTLIBRARIES = libnfcutils.la

libnfcutils_la_SOURCES = nfc-utils.c

nfc_emulate_forum_tag4_SOURCES = nfc-emulate-forum-tag4.c
nfc_emulate_forum_tag4_LDADD = $(top_builddir)/libnfc/libnfc.la \
			       libnfcutils.la

nfc_list_SOURCES = nfc-list.c
nfc_list_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 libnfcutils.la

nfc_mfclassic_SOURCES = nfc-mfclassic.c mifare.c mifare.h
nfc_mfclassic_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la

nfc_mfultralight_SOURCES = nfc-mfultralight.c mifare.c mifare.h
nfc_mfultralight_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_probe_SOURCES = nfc-probe.c
nfc_probe_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 libnfcutils.la

nfc_read_forum_tag3_SOURCES = nfc-read-forum-tag3.c
nfc_read_forum_tag3_LDADD = $(top_builddir)/libnfc/libnfc.la \
		            libnfcutils.la

nfc_relay_picc_SOURCES = nfc-relay-picc.c
nfc_relay_picc_LDADD = $(top_builddir)/libnfc/libnfc.la \
		       libnfcutils.la

dist_man_MANS = \
		nfc-emulate-forum-tag4.1 \
		nfc-list.1 \
		nfc-mfclassic.1 \
		nfc-mfultralight.1 \
		nfc-relay-picc.1

EXTRA_DIST = CMakeLists.txt
