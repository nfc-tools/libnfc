#!/bin/sh

WITH_USB=1

LIBUSB_WIN32_BIN_VERSION="1.2.6.0"
LIBUSB_WIN32_BIN_ARCHIVE="libusb-win32-bin-$LIBUSB_WIN32_BIN_VERSION.zip"
LIBUSB_WIN32_BIN_URL="http://freefr.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/$LIBUSB_WIN32_BIN_VERSION/$LIBUSB_WIN32_BIN_ARCHIVE"
LIBUSB_WIN32_BIN_DIR="libusb-win32-bin-$LIBUSB_WIN32_BIN_VERSION"

if [ "$WITH_USB" = "1" ]; then
  if [ ! -d $LIBUSB_WIN32_BIN_DIR ]; then
    wget -c $LIBUSB_WIN32_BIN_URL
    unzip $LIBUSB_WIN32_BIN_ARCHIVE
  fi
fi

MINGW="${MINGW:=i686-w64-mingw32}"
MINGW_DIR="/usr/$MINGW"

# Use MinGW binaries before others
#export PATH=$MINGW_DIR/bin:$PATH

# Set CPATH to MinGW include files
export CPATH=$MINGW_DIR/include
export LD_LIBRARY_PATH=$MINGW_DIR/lib
export LD_RUN_PATH=$MINGW_DIR/lib

# Force pkg-config to search in cross environement directory
export PKG_CONFIG_LIBDIR=$MINGW_DIR/lib/pkgconfig

# Stop compilation on first error
export CFLAGS="-Wfatal-errors"

# Include default MinGW include directory, and libnfc's win32 files
export CFLAGS="$CFLAGS -I$MINGW_DIR/include -I$PWD/contrib/win32"

if [ "$MINGW" = "i686-w64-mingw32" ]; then
  # mingw-64 includes winscard.a and winscard.h
  #
  # It is not enough to set libpcsclite_LIBS to "-lwinscard", because it is
  # forgotten when libnfc is created with libtool. That's why we are setting
  # LIBS.
  export LIBS="-lwinscard"

  echo "MinGW-w64 ships all requirements libnfc."
  echo "Unfortunately the MinGW-w64 header are currently"
  echo "buggy. Also, Libtool doesn't support MinGW-w64"
  echo "very well."
  echo ""
  echo "Warning ________________________________________"
  echo "You will only be able to compile libnfc.dll, but"
  echo "none of the executables (see utils and examples)."
  echo ""
  # You can fix winbase.h by adding the following lines:
  # #include <basetsd.h>
  # #include <windef.h>
  # But the problem with Libtool remains.
else
  if [ -z "$libpcsclite_LIBS$libpcsclite_CFLAGS" ]; then
  echo "Error __________________________________________"
  echo "You need to get the PC/SC library from a Windows"
  echo "machine and the appropriate header files. Then"
  echo "specify libpcsclite_LIBS=.../WinScard.dll and"
  echo "libpcsclite_CFLAGS=-I..."
  fi
  exit 1
fi

## Configure to cross-compile using mingw32msvc
if [ "$WITH_USB" = "1" ]; then
  # with direct-USB drivers (use libusb-win32)
  DRIVERS="all"
else
  # with UART divers only (can be tested under wine)
  DRIVERS="pn532_uart,arygon"
fi

if [ ! -x configure ]; then
  autoreconf -is
fi

./configure --target=$MINGW --host=$MINGW \
  --with-drivers=$DRIVERS \
  --with-libusb-win32=$PWD/$LIBUSB_WIN32_BIN_DIR \
  $*

if [ "$MINGW" = "i686-w64-mingw32" ]; then
  # due to the buggy headers from MINGW-64 we always add "contrib/windows.h",
  # otherwise some windows types won't be available.
  echo "#include \"contrib/windows.h\"" >> config.h
fi
