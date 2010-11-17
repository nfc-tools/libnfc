#!/bin/sh

WITH_USB=0

LIBUSB_WIN32_BIN_VERSION="1.2.2.0"
LIBUSB_WIN32_BIN_ARCHIVE="libusb-win32-bin-$LIBUSB_WIN32_BIN_VERSION.zip"
LIBUSB_WIN32_BIN_URL="http://freefr.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/$LIBUSB_WIN32_BIN_VERSION/$LIBUSB_WIN32_BIN_ARCHIVE"
LIBUSB_WIN32_BIN_DIR="libusb-win32-bin-$LIBUSB_WIN32_BIN_VERSION"

if [ "$WITH_USB" = "1" ]; then
  if [ ! -d $LIBUSB_WIN32_BIN_DIR ]; then
    wget -c $LIBUSB_WIN32_BIN_URL
    unzip $LIBUSB_WIN32_BIN_ARCHIVE
  fi
fi

MINGW_DIR="/usr/i586-mingw32msvc"

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

## Configure to cross-compile using mingw32msvc
if [ "$WITH_USB" = "1" ]; then
  # with direct-USB drivers (use libusb-win32)
  ./configure --target=i586-mingw32msvc --host=i586-mingw32msvc --with-drivers=pn531_usb,pn533_usb,pn532_uart,arygon --with-libusb-win32=$PWD/$LIBUSB_WIN32_BIN_DIR $*
else
  # with UART divers only (can be tested under wine)
  ./configure --target=i586-mingw32msvc --host=i586-mingw32msvc --with-drivers=pn532_uart,arygon $*
fi
