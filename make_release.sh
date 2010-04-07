#! /bin/sh

# Stop script on first error.
set -e

# Retrieve libnfc version from configure.ac
LIBNFC_VERSION=$(grep AC_INIT configure.ac | sed 's/^.*(\(.*\))/\1/g' | awk -F', ' '{ print $2 }')

# Easiest part: GNU/linux, BSD and other POSIX systems.
LIBNFC_AUTOTOOLS_ARCHIVE=libnfc-$LIBNFC_VERSION.tar.gz

if [ ! -f $LIBNFC_AUTOTOOLS_ARCHIVE ]; then
	echo "Autotooled archive generation..."
	# First, clean what we can
	autoreconf -is && ./configure && make distclean

	# Second, generate dist archive (and test it)
	autoreconf -is && ./configure  && make distcheck

	# Finally, clean up
	make distclean
	echo "Autotooled archive generate."
else
	echo "Autotooled archive (GNU/Linux, BSD, etc.) is already done: skipped."
fi

# Documentation part
LIBNFC_DOC_DIR=libnfc-doc-$LIBNFC_VERSION
LIBNFC_DOC_ARCHIVE=$LIBNFC_DOC_DIR.zip

if [ ! -f $LIBNFC_DOC_ARCHIVE ]; then
	echo "Documentation archive generation..."
	if [ -d $LIBNFC_DOC_DIR ]; then
		rm -rf $LIBNFC_DOC_DIR
	fi

	# Build documentation
	autoreconf -is && ./configure --enable-doc && make doc || false

	# Create archive
	cp -r doc/html $LIBNFC_DOC_DIR
	zip -r $LIBNFC_DOC_ARCHIVE $LIBNFC_DOC_DIR

	# Clean up
	rm -rf $LIBNFC_DOC_DIR
	make distclean
	echo "Documentation archive generated."
else
	echo "Documentation archive is already done: skipped."
fi

# Windows part
LIBNFC_WINDOWS_DIR=libnfc-$LIBNFC_VERSION-winsdk
LIBNFC_WINDOWS_ARCHIVE=$LIBNFC_WINDOWS_DIR.zip

if [ ! -f $LIBNFC_WINDOWS_ARCHIVE ]; then
	echo "Windows archive generation..."
	if [ -d $LIBNFC_WINDOWS_DIR ]; then
		rm -rf $LIBNFC_WINDOWS_DIR
	fi

	mkdir -p $LIBNFC_WINDOWS_DIR

	# Export sources
	svn export libnfc $LIBNFC_WINDOWS_DIR/src
	# Export windows files
	svn export win32 $LIBNFC_WINDOWS_DIR/win32

	# Copy important files
	cp AUTHORS $LIBNFC_WINDOWS_DIR/AUTHORS.txt
	cp LICENSE $LIBNFC_WINDOWS_DIR/LICENSE.txt

	# Remove Autotools Makefile.am
	find $LIBNFC_WINDOWS_DIR/ -name Makefile.am | xargs rm -f

	# Remove CMakeLists.txt
	find $LIBNFC_WINDOWS_DIR/ -name CMakeLists.txt | xargs rm -f

	# Build archive
	zip -r $LIBNFC_WINDOWS_ARCHIVE $LIBNFC_WINDOWS_DIR
	rm -rf $LIBNFC_WINDOWS_DIR
	echo "Windows archive generated."
else
	echo "Windows archive is already done: skipped."
fi

