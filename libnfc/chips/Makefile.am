
# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = pn53x.h pn53x-internal.h
noinst_LTLIBRARIES = libnfcchips.la
libnfcchips_la_SOURCES = pn53x.c
libnfcchips_la_CFLAGS = -I$(top_srcdir)/libnfc


if HAS_LOG4C
  libnfcchips_la_CFLAGS += @log4c_CFLAGS@
  libnfcchips_la_LIBADD = @log4c_LIBS@
endif

