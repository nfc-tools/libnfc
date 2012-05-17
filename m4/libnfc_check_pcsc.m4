dnl Check for PCSC presence (if required)
dnl On success, HAVE_PCSC is set to 1 and PKG_CONFIG_REQUIRES is filled when
dnl libpcsclite is found using pkg-config

AC_DEFUN([LIBNFC_CHECK_PCSC],
[
  if test "x$pcsc_required" = "xyes"; then
    PKG_CHECK_MODULES([libpcsclite], [libpcsclite], [HAVE_PCSC=1], [HAVE_PCSC=0])
    if test x"$HAVE_PCSC" = "x1" ; then
      if test x"$PKG_CONFIG_REQUIRES" != x""; then
        PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
      fi
      PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES libpcsclite"
    fi
    case "$host" in
      *darwin*)
        if test x"$HAVE_PCSC" = "x0" ; then
          AC_MSG_CHECKING(for PC/SC)
          libpcsclite_LIBS="-Wl,-framework,PCSC"
          libpcsclite_CFLAGS=""
          HAVE_PCSC=1
          AC_MSG_RESULT(yes: darwin PC/SC framework)
        fi
      ;;
      *mingw*)
        dnl FIXME Find a way to cross-compile for Windows
        HAVE_PCSC=0
        AC_MSG_RESULT(no: Windows PC/SC framework)
      ;;
      *)
        if test x"$HAVE_PCSC" = "x0" ; then
          AC_MSG_ERROR([libpcsclite is required for building the acr122_pcsc driver.])
        fi
      ;;
    esac
    AC_SUBST(libpcsclite_LIBS)
    AC_SUBST(libpcsclite_CFLAGS)
  fi
])
