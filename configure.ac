# General init
AC_INIT(libnfc, 1.2.1, info@libnfc.org)

AC_CONFIG_HEADER(config.h)

# SVN revison
define([svn_revision], esyscmd([sh -c "svnversion -n"]))
SVN_REVISION=svn_revision
AC_DEFINE_UNQUOTED([SVN_REVISION], ["$SVN_REVISION"], [SVN revision])

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

AC_PATH_PROG(PKG_CONFIG, pkg-config)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h stddef.h stdbool.h])

# Checks for types
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT32_T


# Debug support (default:no)
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Enable debug output]),[enable_debug=$enableval],[enable_debug="no"])
 
AC_MSG_CHECKING(for debug flag)
AC_MSG_RESULT($enable_debug)
 
if test x"$enable_debug" = "xyes"
then
  CFLAGS="$CFLAGS -g -Wall -DDEBUG -pedantic"
fi
AC_SUBST([DEBUG_CFLAGS])

# Serial autoprobing support (default:yes)
AC_ARG_ENABLE([serial-autoprobe],AS_HELP_STRING([--disable-serial-autoprobe],[Disable serial autoprobing]),[enable_serial_autoprobe=$enableval],[enable_serial_autoprobe="yes"])
 
AC_MSG_CHECKING(for serial autoprobe flag)
AC_MSG_RESULT($enable_serial_autoprobe)

if test x"$enable_serial_autoprobe" = "xno"
then
  CFLAGS="$CFLAGS -DDISABLE_SERIAL_AUTOPROBE"
fi

# PCSC-lite support (default: yes)
AC_ARG_ENABLE([pcsc-lite],AS_HELP_STRING([--disable-pcsc-lite],[Disable PCSC-lite dependency (removes ACR122 support)]),[enable_pcsc_lite=$enableval],[enable_pcsc_lite="yes"])
 
AC_MSG_CHECKING(for pcsc-lite support)
AC_MSG_RESULT($enable_pcsc_lite)
 
if test x"$enable_pcsc_lite" = "xno"
then
  WITH_PCSC=0
fi
AM_CONDITIONAL(PCSC_LITE_ENABLED, [test x"$enable_pcsc_lite" = xyes])

# libusb support (default: yes)
AC_ARG_ENABLE([libusb],AS_HELP_STRING([--disable-libusb],[Disable libusb dependency (removes PN531USB and PN533USB support)]),[enable_libusb=$enableval],[enable_libusb="yes"])
 
AC_MSG_CHECKING(for libusb support)
AC_MSG_RESULT($enable_libusb)
 
if test x"$enable_libusb" = "xno"
then
  WITH_LIBUSB=0
fi
AM_CONDITIONAL(LIBUSB_ENABLED, [test x"$enable_libusb" = xyes])

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

AC_ARG_WITH([os-version], [AS_HELP_STRING([--with-os-version], [For FreeBSD users, use this option to override the detected OSVERSION.])])

case "$build_os" in
  freebsd*)
    AC_MSG_CHECKING([the FreeBSD operating system release])
    if test -n "$with_os_version"; then
      OSVERSION="$with_os_version"
    else
      OSVERSION=`/sbin/sysctl -n kern.osreldate`
    fi
    AC_MSG_RESULT([$OSVERSION])
    AC_MSG_CHECKING([which usb library to use])
    if test "$OSVERSION" -lt "800069"; then
      system_has_libusb="no"
      AC_MSG_RESULT([devel/libusb])
    else
      system_has_libusb="yes"
      AC_MSG_RESULT(system)
      LIBUSB_LIBS="-lusb"
    fi
esac


# Dependencies
PKG_CONFIG_REQUIRES=""
## libusb
if test "x$enable_libusb" = "xyes"; then
  if test x"$system_has_libusb" != "xyes"; then
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
    if test x"$PKG_CONFIG_REQUIRES" != x""; then
      PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
    fi
    PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES libusb"
  fi
  
  if test x"$WITH_USB" = "x0"; then
    AC_MSG_ERROR([libusb is mandatory.])
  fi
  fi
  AC_SUBST(LIBUSB_LIBS)
  AC_SUBST(LIBUSB_CFLAGS)
fi

## libpcsclite
if test "x$enable_pcsc_lite" = "xyes"; then
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
      if test x"$PKG_CONFIG_REQUIRES" != x""; then
        PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
      fi
      PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES libpcsclite"
    ;;
  esac
  AC_SUBST(LIBPCSCLITE_LIBS)
  AC_SUBST(LIBPCSCLITE_CFLAGS)
fi

AC_SUBST(PKG_CONFIG_REQUIRES)

# Set C standard to C99
# C99 standard support (default: yes)
AC_ARG_ENABLE([std-c99],AS_HELP_STRING([--disable-std-c99],[Disable C99 standard when compiling]),[enable_std_c99=$enableval],[enable_std_c99="yes"])
 
AC_MSG_CHECKING(for C99 support)
AC_MSG_RESULT($enable_std_c99)
 
if test x"$enable_std_c99" != "xno"
then
	CFLAGS="$CFLAGS -std=c99"

	# Workarounds for libusb in c99
	CFLAGS="$CFLAGS -Du_int8_t=uint8_t -Du_int16_t=uint16_t"
fi

AC_CONFIG_FILES([
		Makefile
		src/lib/chips/Makefile
		src/lib/buses/Makefile
		src/lib/drivers/Makefile
		src/lib/Makefile
		src/examples/Makefile
		src/Makefile
		libnfc.pc
		Doxyfile
		])

AC_OUTPUT
