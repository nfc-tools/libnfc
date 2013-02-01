SUBDIRS = pn53x-tamashell-scripts

bin_PROGRAMS = \
		nfc-anticol \
		nfc-dep-initiator \
		nfc-dep-target \
		nfc-emulate-forum-tag2 \
		nfc-emulate-tag \
		nfc-emulate-uid \
		nfc-mfsetuid \
		nfc-poll \
		nfc-relay \
		pn53x-diagnose \
		pn53x-sam

if POSIX_ONLY_EXAMPLES_ENABLED
bin_PROGRAMS += \
		pn53x-tamashell
endif

check_PROGRAMS = \
		quick_start_example1 \
		quick_start_example2

# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

AM_CFLAGS = -I$(top_srcdir)/libnfc -I$(top_srcdir)

nfc_poll_SOURCES = nfc-poll.c
nfc_poll_LDADD = $(top_builddir)/libnfc/libnfc.la \
		 $(top_builddir)/utils/libnfcutils.la

nfc_anticol_SOURCES = nfc-anticol.c
nfc_anticol_LDADD = $(top_builddir)/libnfc/libnfc.la \
		    $(top_builddir)/utils/libnfcutils.la

nfc_relay_SOURCES = nfc-relay.c
nfc_relay_LDADD = $(top_builddir)/libnfc/libnfc.la \
		  $(top_builddir)/utils/libnfcutils.la

nfc_emulate_forum_tag2_SOURCES = nfc-emulate-forum-tag2.c
nfc_emulate_forum_tag2_LDADD = $(top_builddir)/libnfc/libnfc.la \
			$(top_builddir)/utils/libnfcutils.la

nfc_emulate_tag_SOURCES = nfc-emulate-tag.c
nfc_emulate_tag_LDADD = $(top_builddir)/libnfc/libnfc.la \
			$(top_builddir)/utils/libnfcutils.la

nfc_emulate_uid_SOURCES = nfc-emulate-uid.c
nfc_emulate_uid_LDADD = $(top_builddir)/libnfc/libnfc.la \
		        $(top_builddir)/utils/libnfcutils.la

nfc_dep_target_SOURCES = nfc-dep-target.c
nfc_dep_target_LDADD = $(top_builddir)/libnfc/libnfc.la \
		       $(top_builddir)/utils/libnfcutils.la

nfc_dep_initiator_SOURCES = nfc-dep-initiator.c
nfc_dep_initiator_LDADD = $(top_builddir)/libnfc/libnfc.la \
			  $(top_builddir)/utils/libnfcutils.la

nfc_mfsetuid_SOURCES = nfc-mfsetuid.c
nfc_mfsetuid_LDADD = $(top_builddir)/libnfc/libnfc.la \
			  $(top_builddir)/utils/libnfcutils.la

pn53x_diagnose_SOURCES = pn53x-diagnose.c
pn53x_diagnose_LDADD = $(top_builddir)/libnfc/libnfc.la \
		       $(top_builddir)/utils/libnfcutils.la

pn53x_sam_SOURCES = pn53x-sam.c
pn53x_sam_LDADD = $(top_builddir)/libnfc/libnfc.la \
		  $(top_builddir)/utils/libnfcutils.la

pn53x_tamashell_SOURCES = pn53x-tamashell.c
pn53x_tamashell_LDADD = $(top_builddir)/libnfc/libnfc.la \
		        $(top_builddir)/utils/libnfcutils.la
pn53x_tamashell_LDFLAGS = @READLINE_LIBS@

quick_start_example1_SOURCES = doc/quick_start_example1.c
quick_start_example1_LDADD =  $(top_builddir)/libnfc/libnfc.la \
		  $(top_builddir)/utils/libnfcutils.la

quick_start_example2_SOURCES = doc/quick_start_example2.c
quick_start_example2_LDADD =  $(top_builddir)/libnfc/libnfc.la \
		  $(top_builddir)/utils/libnfcutils.la


dist_man_MANS = \
		nfc-anticol.1 \
		nfc-dep-initiator.1 \
		nfc-dep-target.1 \
		nfc-emulate-tag.1 \
		nfc-emulate-uid.1 \
		nfc-poll.1 \
		nfc-relay.1 \
		nfc-mfsetuid.1 \
		pn53x-diagnose.1 \
		pn53x-sam.1 \
		pn53x-tamashell.1 \
		nfc-emulate-forum-tag2.1

EXTRA_DIST = CMakeLists.txt
