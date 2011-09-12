
# set the include path found by configure
INCLUDES= $(all_includes) $(LIBNFC_CFLAGS)

noinst_HEADERS = uart.h
noinst_LTLIBRARIES = libnfcbuses.la
libnfcbuses_la_SOURCES = uart.c
libnfcbuses_la_CFLAGS = -I$(top_srcdir)/libnfc

EXTRA_DIST = uart_posix.c uart_win32.c

if HAS_LOG4C
  libnfcbuses_la_CFLAGS += @log4c_CFLAGS@
  libnfcbuses_la_LIBADD = @log4c_LIBS@
endif
