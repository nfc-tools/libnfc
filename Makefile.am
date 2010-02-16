ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src cmake_modules

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
