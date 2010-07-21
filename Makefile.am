ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(LIBNFC_CFLAGS)

SUBDIRS = libnfc examples include cmake_modules

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc.pc

EXTRA_DIST = Doxyfile pn53x.conf pn53x.rules CMakeLists.txt config.h.cmake LICENSE
CLEANFILES = Doxygen.log

if DOC_ENABLED
clean-local:
	rm -rf doc

doc : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

.PHONY: doc
endif

if HAS_INDENT
INDENT_ARGS = -br -brs -ce --line-length120 -nut -i2
indent:
	find $(top_srcdir)/libnfc $(top_srcdir)/include $(top_srcdir)/examples -name "*.[hc]" -exec $(INDENT) $(INDENT_ARGS) '{}' ';'
endif
