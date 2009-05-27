AC_INIT(libnfc, 1.1.1, roel@libnfc.org)

AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

AC_LANG_C
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_PATH_PROG(PKG_CONFIG, pkg-config)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h stddef.h])


# libusb
PKG_CHECK_MODULES(LIBUSB, libusb, [WITH_USB=1], [WITH_USB=0])
if test "$WITH_USB" == "0" ; then
  AC_MSG_ERROR([libusb is mandatory.])
fi
AC_SUBST(LIBUSB_LIBS)
AC_SUBST(LIBUSB_CFLAGS)

# libpcsclite
PKG_CHECK_MODULES(LIBPCSCLITE, libpcsclite, [WITH_PCSC=1], [WITH_PCSC=0])
if test "$WITH_PCSC" == "0" ; then
  AC_MSG_ERROR([libpcsclite is mandatory.])
fi
AC_SUBST(LIBPCSCLITE_LIBS)
AC_SUBST(LIBPCSCLITE_CFLAGS)

# --enable-debug support
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Debug flags]),[enable_debug=$enableval],[enable_debug="no"])
 
AC_MSG_CHECKING(debug)
AC_MSG_RESULT($enable_debug)
 
if test "x$enable_debug" = "xyes"
then
        CFLAGS="$CFLAGS -g -Wall -DDEBUG -pedantic"
fi
AC_SUBST([DEBUG_CFLAGS])

CFLAGS="$CFLAGS -std=c99"

AC_CONFIG_FILES([
		Makefile
		src/Makefile
		libnfc.pc
		])

AC_OUTPUT
