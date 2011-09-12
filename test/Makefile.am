# $Id$

INCLUDES = $(CUTTER_CFLAGS) $(LIBNFC_CFLAGS)
LIBS = $(CUTTER_LIBS)

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

cutter_unit_test_libs = \
			test_access_storm.la \
			test_dep.la \
			test_register_access.la \
			test_register_endianness.la

if WITH_DEBUG
noinst_LTLIBRARIES = $(cutter_unit_test_libs)
else
check_LTLIBRARIES = $(cutter_unit_test_libs)
endif

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_access_storm_la_SOURCES = test_access_storm.c
test_access_storm_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

test_dep_la_SOURCES = test_dep.c
test_dep_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

test_register_access_la_SOURCES = test_register_access.c
test_register_access_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

test_register_endianness_la_SOURCES = test_register_endianness.c
test_register_endianness_la_LIBADD = $(top_builddir)/libnfc/libnfc.la

echo-cutter:
		@echo $(CUTTER)

EXTRA_DIST = run-test.sh
CLEANFILES = *.gcno

endif
