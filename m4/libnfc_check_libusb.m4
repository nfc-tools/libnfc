dnl Check for LIBUSB
dnl On success, HAVE_LIBUSB is set to 1 and PKG_CONFIG_REQUIRES is filled when
dnl libusb is found using pkg-config

AC_DEFUN([LIBNFC_CHECK_LIBUSB],
[
  if test x"$libusb_required" = "xyes"; then
    HAVE_LIBUSB=0

    # Search using libusb module using pkg-config
    if test x"$HAVE_LIBUSB" = "x0"; then  
      if test x"$PKG_CONFIG" != "x"; then
        PKG_CHECK_MODULES([libusb], [libusb-1.0], [HAVE_LIBUSB=1], [HAVE_LIBUSB=0])
        if test x"$HAVE_LIBUSB" = "x1"; then
          if test x"$PKG_CONFIG_REQUIRES" != x""; then
            PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
          fi
          PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES libusb"
        fi
      fi
    fi

    # Search the library and headers directly (last chance)
    if test x"$HAVE_LIBUSB" = "x0"; then
      AC_CHECK_HEADER(libusb.h, [], [AC_MSG_ERROR([The libusb headers are missing])])
      AC_CHECK_LIB(usb-1.0, libusb_init, [], [AC_MSG_ERROR([The libusb library is missing])])
  
      libusb_LIBS="-lusb"
      HAVE_LIBUSB=1
    fi

    if test x"$HAVE_LIBUSB" = "x0"; then
      AC_MSG_ERROR([libusb is mandatory.])
    fi

    AC_SUBST(libusb_LIBS)
    AC_SUBST(libusb_CFLAGS)
  fi
])
