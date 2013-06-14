ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(LIBNFC_CFLAGS)

SUBDIRS = libnfc utils examples include contrib cmake test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc.pc

EXTRA_DIST = \
	CMakeLists.txt \
	Doxyfile \
	README-Windows.txt \
	libnfc.conf.sample

CLEANFILES = Doxygen.log coverage.info libnfc.pc

clean-local: clean-local-doc clean-local-coverage

.PHONY: clean-local-coverage clean-local-doc doc style
clean-local-coverage:
	-rm -rf coverage

clean-local-doc:
	rm -rf doc

doc : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

DISTCHECK_CONFIGURE_FLAGS="--with-drivers=all"

style:
	find . -name "*.[ch]" -exec perl -pi -e 's/[ \t]+$$//' {} \;
	find . -name "*.[ch]" -exec astyle --formatted --mode=c --suffix=none \
	    --indent=spaces=2 --indent-switches --indent-preprocessor \
	    --keep-one-line-blocks --max-instatement-indent=60 \
	    --brackets=linux --pad-oper --unpad-paren --pad-header \
	    --align-pointer=name {} \;

cppcheck:
	cppcheck --quiet \
	    -I include -I libnfc -I libnfc/buses -I libnfc/chips -I libnfc/drivers \
	    --check-config .
	cppcheck --quiet --enable=all --std=posix --std=c99 \
	    -I include -I libnfc -I libnfc/buses -I libnfc/chips -I libnfc/drivers \
	    -DLOG -D__linux__ \
	    -DDRIVER_PN53X_USB_ENABLED -DDRIVER_ACR122_PCSC_ENABLED \
	    -DDRIVER_ACR122_USB_ENABLED -DDRIVER_ACR122S_ENABLED \
	    -DDRIVER_PN532_UART_ENABLED -DDRIVER_ARYGON_ENABLED \
	    -DDRIVER_PN532_SPI_ENABLED -DDRIVER_PN532_I2C_ENABLED \
	    --force --inconclusive .
