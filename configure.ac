# General init

# /!\ Don't forget to update 'CMakeLists.txt' too /!\
AC_INIT([libnfc],[1.7.1],[nfc-tools@googlegroups.com])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADER(config.h)

# GIT revison
GIT_REVISION=`which git > /dev/null && git describe`
if test x"$GIT_REVISION" != x""; then
    AC_DEFINE_UNQUOTED([GIT_REVISION], ["$GIT_REVISION"], [GIT revision])
fi

AM_INIT_AUTOMAKE(subdir-objects dist-bzip2 no-dist-gzip)
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG([C])
AC_PROG_CC
AC_PROG_MAKE_SET

WITH_POSIX_ONLY_EXAMPLES=1
# Libtool
LT_INIT
case "$host" in
  *-pc-linux-gnu)
    AC_MSG_RESULT([Fixing libtool for -rpath problems.])
    sed -i -r 's/(hardcode_into_libs)=.*$/\1=no/' libtool
  ;;
  *-pc-mingw32msvc)
    WITH_POSIX_ONLY_EXAMPLES=0
    AC_MSG_RESULT([Disable extended examples due to target Windows system.])
    # Undefine __STRICT_ANSI__ to allow to use strdup, putenv, etc. without warnings
    CFLAGS="$CFLAGS -U__STRICT_ANSI__"
   ;;
esac
AM_CONDITIONAL(POSIX_ONLY_EXAMPLES_ENABLED, [test "$WITH_POSIX_ONLY_EXAMPLES" = "1"])

PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h limits.h stdio.h stdlib.h stdint.h stddef.h stdbool.h sys/ioctl.h sys/param.h sys/time.h termios.h])
AC_CHECK_HEADERS([linux/spi/spidev.h], [spi_available="yes"])
AC_CHECK_HEADERS([linux/i2c-dev.h], [i2c_available="yes"])
AC_CHECK_FUNCS([memmove memset select strdup strerror strstr strtol usleep],
	       [AC_DEFINE([_XOPEN_SOURCE], [600], [Enable POSIX extensions if present])])

AC_DEFINE(_NETBSD_SOURCE, 1, [Define on NetBSD to activate all library features])
AC_DEFINE(_DARWIN_C_SOURCE, 1, [Define on Darwin to activate all library features])

# Note: malloc function should be tested but it produces some error while cross-compiling with MinGW
# AC_FUNC_MALLOC

# Checks for types
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_OFF_T

LIBNFC_CFLAGS='-I$(top_srcdir)/libnfc -I$(top_builddir)/include -I$(top_srcdir)/include'
AC_SUBST(LIBNFC_CFLAGS)

# Log support (default:yes)
AC_ARG_ENABLE([log],AS_HELP_STRING([--disable-log],[Disable any logs]),[enable_log=$enableval],[enable_log="yes"])
AC_MSG_CHECKING(for log flag)
AC_MSG_RESULT($enable_log)
AM_CONDITIONAL([WITH_LOG], [test "$enable_log" != "no"])
 
if test x"$enable_log" = "xyes"
then
  AC_DEFINE([LOG], [1], [Enable log])
fi

# Conffiles support (default:yes)
AC_ARG_ENABLE([conffiles],AS_HELP_STRING([--disable-conffiles],[Disable use of config files]),[enable_conffiles=$enableval],[enable_conffiles="yes"])
AC_MSG_CHECKING(for conffiles flag)
AC_MSG_RESULT($enable_conffiles)
AM_CONDITIONAL([WITH_CONFFILES], [test "$enable_conffiles" != "no"])
 
if test x"$enable_conffiles" = "xyes"
then
  AC_DEFINE([CONFFILES], [1], [Enable conffiles])
fi

# Envvars support (default:yes)
AC_ARG_ENABLE([envvars],AS_HELP_STRING([--disable-envvars],[Disable use of environment variables]),[enable_envvars=$enableval],[enable_envvars="yes"])
AC_MSG_CHECKING(for envvars flag)
AC_MSG_RESULT($enable_envvars)
AM_CONDITIONAL([WITH_ENVVARS], [test "$enable_envvars" != "no"])
 
if test x"$enable_envvars" = "xyes"
then
  AC_DEFINE([ENVVARS], [1], [Enable envvars])
fi

# Debug support (default:no)
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Enable debug mode]),[enable_debug=$enableval],[enable_debug="no"])
AC_MSG_CHECKING(for debug flag)
AC_MSG_RESULT($enable_debug)
AM_CONDITIONAL([WITH_DEBUG], [test "$enable_debug" != "no"])
 
if test x"$enable_debug" = "xyes"
then
  AC_DEFINE([DEBUG], [1], [Enable debug flag])
  CFLAGS="$CFLAGS -g -O0 -ggdb"
fi

# Handle --with-drivers option
LIBNFC_ARG_WITH_DRIVERS

# Enable UART if
AM_CONDITIONAL(UART_ENABLED, [test x"$uart_required" = x"yes"])

# Enable SPI if
AM_CONDITIONAL(SPI_ENABLED, [test x"$spi_required" = x"yes"])

# Enable I2C if 
AM_CONDITIONAL(I2C_ENABLED, [test x"$i2c_required" = x"yes"])

# Documentation (default: no)
AC_ARG_ENABLE([doc],AS_HELP_STRING([--enable-doc],[Enable documentation generation.]),[enable_doc=$enableval],[enable_doc="no"])

AC_MSG_CHECKING(for documentation request)
AC_MSG_RESULT($enable_doc)

if test x"$enable_doc" = "xyes"
then
  AC_PATH_PROG([DOXYGEN], [doxygen])
  if test x$DOXYGEN = x
  then
    AC_MSG_ERROR([doxygen is mandatory.])
  fi
fi
AM_CONDITIONAL(DOC_ENABLED, [test x"$enable_doc" = xyes])

# Dependencies
PKG_CONFIG_REQUIRES=""

LIBNFC_CHECK_LIBUSB
LIBNFC_CHECK_PCSC

AC_SUBST(PKG_CONFIG_REQUIRES)

AM_CONDITIONAL(LIBUSB_ENABLED, [test "$HAVE_LIBUSB" = "1"])
AM_CONDITIONAL(PCSC_ENABLED, [test "$HAVE_PCSC" = "1"])

CUTTER_REQUIRED_VERSION=1.1.7
m4_ifdef([AC_CHECK_CUTTER], [AC_CHECK_CUTTER([>= $CUTTER_REQUIRED_VERSION])], [ac_cv_use_cutter="no"])
if test x$ac_cv_with_cutter = xyes -a x$ac_cv_use_cutter = xno; then
  AC_MSG_ERROR([cutter >= $CUTTER_REQUIRED_VERSION is mandatory.])
fi
AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])

AC_CHECK_READLINE

# Help us to write great code ;-)
CFLAGS="$CFLAGS -Wall -pedantic -Wextra"

# Defines and C flags
CFLAGS="$CFLAGS -std=c99"

# Workarounds for libusb in c99
CFLAGS="$CFLAGS -Du_int8_t=uint8_t -Du_int16_t=uint16_t"

AC_CONFIG_FILES([
		Doxyfile
		Makefile
		cmake/Makefile
		cmake/modules/Makefile
		contrib/Makefile
		contrib/devd/Makefile
		contrib/libnfc/Makefile
		contrib/linux/Makefile
		contrib/udev/Makefile
		contrib/win32/Makefile
		contrib/win32/sys/Makefile
		contrib/win32/libnfc/Makefile
		contrib/win32/libnfc/buses/Makefile
		examples/Makefile
		examples/pn53x-tamashell-scripts/Makefile
		include/Makefile
		include/nfc/Makefile
		libnfc.pc
		libnfc/Makefile
		libnfc/buses/Makefile
		libnfc/chips/Makefile
		libnfc/drivers/Makefile
		test/Makefile
		utils/Makefile
		])

AC_OUTPUT

LIBNFC_DRIVERS_SUMMARY
