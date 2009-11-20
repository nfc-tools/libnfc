SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc.pc

EXTRA_DIST = Doxyfile pn53x.rules
CLEANFILES = Doxygen.log

if DOC_ENABLED
clean-local:
	rm -rf doc

html-am : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

.PHONY: html
endif
DISTCLEANFILES = \
  Makefile.in \
  configure \
  depcomp \
  config.guess \
  config.sub \
  ltmain.sh \
  config.h.in \
  aclocal.m4 
