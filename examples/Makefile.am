bin_PROGRAMS = \
		nfc-anticol \
		nfc-emulate \
		nfc-emulate-ndef \
		nfc-list \
		nfc-mfclassic \
		nfc-mfultralight \
		nfc-poll \
		nfc-relay \
		nfc-sam \
		nfcip-initiator \
		nfcip-target \
		pn53x-diagnose \
		pn53x-tamashell

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

nfc_emulate_SOURCES = nfc-emulate.c
nfc_emulate_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la

nfc_emulate_ndef_SOURCES = nfc-emulate-ndef.c
nfc_emulate_ndef_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la

nfcip_target_SOURCES = nfcip-target.c
nfcip_target_LDADD = $(top_builddir)/libnfc/libnfc.la

nfcip_initiator_SOURCES = nfcip-initiator.c
nfcip_initiator_LDADD = $(top_builddir)/libnfc/libnfc.la

pn53x_diagnose_SOURCES = pn53x-diagnose.c
pn53x_diagnose_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la

pn53x_tamashell_SOURCES = pn53x-tamashell.c
pn53x_tamashell_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    libnfcutils.la
# Should be guarded by test on availability of readline
#pn53x_tamashell_CFLAGS = -DHAVE_READLINE
#pn53x_tamashell_LDFLAGS = -lreadline

nfc_sam_SOURCES = nfc-sam.c
nfc_sam_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 libnfcutils.la

dist_man_MANS = nfc-anticol.1 nfc-emulate.1 nfc-list.1 nfc-mfclassic.1 nfc-mfultralight.1 nfc-relay.1 nfc-sam.1

EXTRA_DIST = CMakeLists.txt
