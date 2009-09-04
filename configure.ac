AC_INIT(libnfc, 1.2.1, info@libnfc.org)

AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

AC_LANG_C
AC_PROG_CC
AC_PROG_MAKE_SET

# Libtool
AC_PROG_LIBTOOL
case "$host" in
   *-pc-linux-gnu)
     AC_MSG_RESULT([Fixing libtool for -rpath problems.])
     sed -i -r 's/(hardcode_into_libs)=.*$/\1=no/' libtool
   ;;
esac

AC_PROG_RANLIB

AC_PATH_PROG(PKG_CONFIG, pkg-config)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h stddef.h stdbool.h usb.h])

# Checks for types
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT32_T

# --enable-debug support (default:no)
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Debug flags]),[enable_debug=$enableval],[enable_debug="no"])
 
AC_MSG_CHECKING(for debug flag)
AC_MSG_RESULT($enable_debug)
 
if test x"$enable_debug" = "xyes"
then
  CFLAGS="$CFLAGS -g -Wall -DDEBUG -pedantic"
fi
AC_SUBST([DEBUG_CFLAGS])

# --enable-serial-autoprobe support (default:yes)
AC_ARG_ENABLE([serial-autoprobe],AS_HELP_STRING([--enable-serial-autoprobe],[Serial autoprobing flag]),[enable_serial_autoprobe=$enableval],[enable_serial_autoprobe="yes"])
 
AC_MSG_CHECKING(for serial autoprobe flag)
AC_MSG_RESULT($enable_serial_autoprobe)

if test x"$enable_serial_autoprobe" = "xno"
then
  CFLAGS="$CFLAGS -DDISABLE_SERIAL_AUTOPROBE"
fi

# --enable-pcsc-lite support (default: yes)
AC_ARG_ENABLE([pcsc-lite],AS_HELP_STRING([--enable-pcsc-lite],[pcsc-lite feature flag]),[enable_pcsc_lite=$enableval],[enable_pcsc_lite="yes"])
 
AC_MSG_CHECKING(for pcsc-lite support)
AC_MSG_RESULT($enable_pcsc_lite)
 
if test x"$enable_pcsc_lite" = "xno"
then
        WITH_PCSC=0
fi
AM_CONDITIONAL(PCSC_LITE_ENABLED, [test x"$enable_pcsc_lite" = xyes])

## libusb
if test x"$PKG_CONFIG" = "x"; then
  AC_PATH_PROG(LIBUSB_CONFIG,libusb-config)
  if test x"$LIBUSB_CONFIG" = "x" ; then
    WITH_USB=0
  else
    LIBUSB_CFLAGS=`$LIBUSB_CONFIG --cflags`
    LIBUSB_LIBS=`$LIBUSB_CONFIG --libs`
    WITH_USB=1
  fi
else
  PKG_CHECK_MODULES(LIBUSB, libusb, [WITH_USB=1], [WITH_USB=0])
fi

if test x"$WITH_USB" = "x0"; then
  AC_MSG_ERROR([libusb is mandatory.])
fi
AC_SUBST(LIBUSB_LIBS)
AC_SUBST(LIBUSB_CFLAGS)

## libpcsclite
if test "x$enable_pcsc_lite" = "xyes"
then
case "$host" in
  *darwin*)
    AC_MSG_WARN(Using darwin PCSC Framework)
    LIBPCSCLITE_LIBS="-Wl,-framework,PCSC"
    LIBPCSCLITE_CFLAGS="-I/System/Library/Frameworks/PCSC.framework/Headers"
  ;;
  *)
    PKG_CHECK_MODULES(LIBPCSCLITE, libpcsclite, [WITH_PCSC=1], [WITH_PCSC=0])
    if test x"$WITH_PCSC" = "x0" ; then
      AC_MSG_ERROR([libpcsclite is mandatory.])
    fi
  ;;
esac
  AC_SUBST(LIBPCSCLITE_LIBS)
  AC_SUBST(LIBPCSCLITE_CFLAGS)
fi

# Set C standard to C99
# --disable-std-c99 support (default: no)
AC_ARG_ENABLE([std-c99],AS_HELP_STRING([--disable-pcsc-lite],[compile using C99 standard]),[enable_std_c99=$enableval],[enable_std_c99="yes"])
 
AC_MSG_CHECKING(for C99 support)
AC_MSG_RESULT($enable_std_c99)
 
if test x"$enable_std_c99" != "xno"
then
	CFLAGS="$CFLAGS -std=c99"
fi

AC_CONFIG_FILES([
		Makefile
		src/Makefile
		libnfc.pc
		])

AC_OUTPUT
