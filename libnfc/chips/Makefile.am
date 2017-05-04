
# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_LTLIBRARIES = libnfcchips.la
libnfcchips_la_SOURCES = pn53x.c pn53x.h pn53x-internal.h rc522.c rc522.h rc522-internal.h
libnfcchips_la_CFLAGS = -I$(top_srcdir)/libnfc

