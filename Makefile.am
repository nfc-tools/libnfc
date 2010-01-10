ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc.pc

EXTRA_DIST = Doxyfile pn53x.rules
CLEANFILES = Doxygen.log

if DOC_ENABLED
clean-local:
	rm -rf doc

doc : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

.PHONY: doc
endif
