ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(LIBNFC_CFLAGS)

SUBDIRS = libnfc examples include contrib cmake_modules test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc.pc

EXTRA_DIST = \
	CMakeLists.txt \
	Doxyfile \
	LICENSE \
	README-Windows.txt \
	config.h.cmake \
	pn53x.conf \
	pn53x.rules

CLEANFILES = Doxygen.log

if DOC_ENABLED
clean-local:
	rm -rf doc

doc : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

.PHONY: doc
endif

if HAS_INDENT
INDENT_ARGS = --blank-before-sizeof \
	      --blank-lines-after-commas \
	      --blank-lines-after-procedures \
	      --braces-after-func-def-line \
	      --braces-on-if-line \
	      --braces-on-struct-decl-line \
	      --continue-at-parentheses \
	      --cuddle-else \
	      --declaration-indentation 8 \
	      --indent-level 2 \
	      --line-length 120 \
	      --no-blank-lines-after-declarations \
	      --no-tabs \
	      --preprocessor-indentation 2 \
	      --procnames-start-lines \
	      --space-after-cast \
	      --space-after-procedure-calls

indent:
	@[ ! -d $(top_srcdir)/.svn ] || \
	    [ 0 -eq $$(svn status | grep '\.[hc]$$' | tee /dev/fd/2 | wc -l) ] || \
	    ( echo "These files are localy modified.  'commit' or 'reverse' them, then retry." >&2; false )
	@for f in $$(find $(top_srcdir)/libnfc $(top_srcdir)/include $(top_srcdir)/examples -name "*.[hc]"); do \
	    echo "  INDENT $$f"; \
	    $(INDENT) $(INDENT_ARGS) "$$f" || exit 1; \
	done
endif

DISTCHECK_CONFIGURE_FLAGS="--with-drivers=all"

