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

# Include default MinGW include directory
export CFLAGS="$CFLAGS -I$MINGW_DIR/include"

if [ "$MINGW" = "i686-w64-mingw32" ]; then
  # mingw-64 includes winscard.a and winscard.h
  #
  # It is not enough to set libpcsclite_LIBS to "-lwinscard", because it is
  # forgotten when libnfc is created with libtool. That's why we are setting
  # LIBS.
  if echo -n "$*" | grep acr122_pcsc 2>&1 > /dev/null; then
    export LIBS="-lwinscard"
  fi
fi

if [ ! -x configure ]; then
  autoreconf -is
fi

./configure --target=$MINGW --host=$MINGW \
  --disable-conffiles --disable-log \
  --with-libusb-win32=$PWD/$LIBUSB_WIN32_BIN_DIR \
  $*
