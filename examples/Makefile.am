SUBDIRS = pn53x-tamashell-scripts

bin_PROGRAMS = \
		nfc-anticol \
		nfc-dep-initiator \
		nfc-dep-target \
		nfc-emulate-forum-tag4 \
		nfc-emulate-tag \
		nfc-emulate-uid \
		nfc-list \
		nfc-mfclassic \
		nfc-mfultralight \
		nfc-poll \
		nfc-relay \
		nfc-relay-picc \
		pn53x-diagnose \
		pn53x-sam

if POSIX_ONLY_EXAMPLES_ENABLED
bin_PROGRAMS += \
		pn53x-tamashell
endif

# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

AM_CFLAGS = -I$(top_srcdir)/libnfc

noinst_HEADERS = mifare.h nfc-utils.h
noinst_LTLIBRARIES = libnfcutils.la

libnfcutils_la_SOURCES = nfc-utils.c

nfc_poll_SOURCES = nfc-poll.c
nfc_poll_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 libnfcutils.la

nfc_anticol_SOURCES = nfc-anticol.c
nfc_anticol_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la

nfc_list_SOURCES = nfc-list.c
nfc_list_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 libnfcutils.la

nfc_mfultralight_SOURCES = nfc-mfultralight.c mifare.c mifare.h
nfc_mfultralight_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_mfclassic_SOURCES = nfc-mfclassic.c mifare.c mifare.h
nfc_mfclassic_LDADD = $(top_builddir)/libnfc/libnfc.la

nfc_relay_SOURCES = nfc-relay.c
nfc_relay_LDADD = $(top_builddir)/libnfc/libnfc.la \
		  libnfcutils.la

nfc_relay_picc_SOURCES = nfc-relay-picc.c
nfc_relay_picc_LDADD = $(top_builddir)/libnfc/libnfc.la \
		  libnfcutils.la

nfc_emulate_forum_tag4_SOURCES = nfc-emulate-forum-tag4.c
nfc_emulate_forum_tag4_LDADD = $(top_builddir)/libnfc/libnfc.la \
			       libnfcutils.la

nfc_emulate_tag_SOURCES = nfc-emulate-tag.c
nfc_emulate_tag_LDADD = $(top_builddir)/libnfc/libnfc.la \
			 libnfcutils.la

nfc_emulate_uid_SOURCES = nfc-emulate-uid.c
nfc_emulate_uid_LDADD = $(top_builddir)/libnfc/libnfc.la \
		        libnfcutils.la

nfc_dep_target_SOURCES = nfc-dep-target.c
nfc_dep_target_LDADD = $(top_builddir)/libnfc/libnfc.la \
		     libnfcutils.la

nfc_dep_initiator_SOURCES = nfc-dep-initiator.c
nfc_dep_initiator_LDADD = $(top_builddir)/libnfc/libnfc.la \
			libnfcutils.la

pn53x_diagnose_SOURCES = pn53x-diagnose.c
pn53x_diagnose_LDADD = $(top_builddir)/libnfc/libnfc.la \
		       libnfcutils.la

pn53x_sam_SOURCES = pn53x-sam.c
pn53x_sam_LDADD = $(top_builddir)/libnfc/libnfc.la \
		libnfcutils.la

pn53x_tamashell_SOURCES = pn53x-tamashell.c
pn53x_tamashell_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la
pn53x_tamashell_LDFLAGS = @READLINE_LIBS@

dist_man_MANS = \
		nfc-anticol.1 \
		nfc-dep-initiator.1 \
		nfc-dep-target.1 \
		nfc-emulate-forum-tag4.1 \
		nfc-emulate-tag.1 \
		nfc-emulate-uid.1 \
		nfc-list.1 \
		nfc-mfclassic.1 \
		nfc-mfultralight.1 \
		nfc-poll.1 \
		nfc-relay.1 \
		nfc-relay-picc.1 \
		pn53x-diagnose.1 \
		pn53x-sam.1 \
		pn53x-tamashell.1

EXTRA_DIST = CMakeLists.txt
