#!/bin/sh
PROJECT_DIR=$(readlink -e $(dirname $0))
cd "$PROJECT_DIR"

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


rm -rf build
mkdir build
cd build


case $1 in
32*)
    mingw32-cmake .. -DLIBUSB_INCLUDE_DIRS="$PROJECT_DIR"/libusb-win32-bin-1.2.6.0/include -DLIBUSB_LIBRARIES="$PROJECT_DIR"/libusb-win32-bin-1.2.6.0/bin/x86/libusb0_x86.dll -DLIBNFC_ROOT_DIR=$PWD/.. -DLIBNFC_SYSCONFDIR='C:\\Program Files (x86)\\libnfc\\config'
    mingw32-make;;
64*)
    mingw64-cmake .. -DLIBUSB_INCLUDE_DIRS="$PROJECT_DIR"/libusb-win32-bin-1.2.6.0/include -DLIBUSB_LIBRARIES="$PROJECT_DIR"/libusb-win32-bin-1.2.6.0/bin/amd64/libusb0.dll -DLIBNFC_ROOT_DIR=.. -DLIBNFC_SYSCONFDIR='C:\\Program Files\\libnfc\\config'
    mingw64-make;;
*)
    echo "specify whether to build 32-bit or 64-bit version by supplying 32 or 64 as parameter";;
esac
