# $Id$

INCLUDES = $(CUTTER_CFLAGS) $(LIBNFC_CFLAGS)
LIBS = $(CUTTER_LIBS)

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = \
		     test_access_storm.la \
		     test_register_access.la \
		     test_register_endianness.la

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_access_storm_la_SOURCES = test_access_storm.c
test_access_storm_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

test_register_access_la_SOURCES = test_register_access.c
test_register_access_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

test_register_endianness_la_SOURCES = test_register_endianness.c
test_register_endianness_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

echo-cutter:
		@echo $(CUTTER)

EXTRA_DIST = run-test.sh
CLEANFILES = *.gcno

endif
