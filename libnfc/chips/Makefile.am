
# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = pn53x.h pn53x-internal.h
noinst_LTLIBRARIES = libnfcchips.la
libnfcchips_la_SOURCES = pn53x.c
libnfcchips_la_CFLAGS = -I$(top_srcdir)/libnfc

