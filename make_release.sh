#! /bin/sh

# Stop script on first error.
set -e

# Retrieve libnfc version from configure.ac
LIBNFC_VERSION=$(grep AC_INIT configure.ac | sed 's/^.*(\(.*\))/\1/g' | awk -F', ' '{ print $2 }')

# Easiest part: GNU/linux, BSD and other POSIX systems.
LIBNFC_AUTOTOOLS_ARCHIVE=libnfc-$LIBNFC_VERSION.tar.gz

if [ ! -f $LIBNFC_AUTOTOOLS_ARCHIVE ]; then
	# First, we can test archive using "distcheck"
	autoreconf -vis && ./configure && make distcheck

	# Clean up
        make distclean

	# We are ready to make a good autotools release.
	autoreconf -vis && ./configure && make dist

	# Clean up
        make distclean
else
	echo "Autotooled archive (GNU/Linux, BSD, etc.) is already done: skipped."
fi

# Documentation part
LIBNFC_DOC_DIR=libnfc-doc-$LIBNFC_VERSION
LIBNFC_DOC_ARCHIVE=$LIBNFC_DOC_DIR.zip

if [ ! -f $LIBNFC_DOC_ARCHIVE ]; then
        if [ -d $LIBNFC_DOC_DIR ]; then
                rm -rf $LIBNFC_DOC_DIR
        fi

	# Build documentation
	autoreconf -vis && ./configure --enable-doc && make doc

	# Create archive
	cp -r doc/html $LIBNFC_DOC_DIR
	zip -r $LIBNFC_DOC_ARCHIVE $LIBNFC_DOC_DIR

	# Clean up
        make distclean
else
	echo "Documentation archive is already done: skipped."
fi

# Windows part
LIBNFC_WINDOWS_DIR=libnfc-$LIBNFC_VERSION-winsdk
LIBNFC_WINDOWS_ARCHIVE=$LIBNFC_WINDOWS_DIR.zip

if [ ! -f $LIBNFC_WINDOWS_ARCHIVE ]; then
	if [ -d $LIBNFC_WINDOWS_DIR ]; then
		rm -rf $LIBNFC_WINDOWS_DIR
	fi

	mkdir -p $LIBNFC_WINDOWS_DIR

	# Export sources
	svn export src $LIBNFC_WINDOWS_DIR/src
	# Export windows files
	svn export win32 $LIBNFC_WINDOWS_DIR/win32

	# Copy important files
	cp AUTHORS $LIBNFC_WINDOWS_DIR/AUTHORS.txt
	cp LICENSE $LIBNFC_WINDOWS_DIR/LICENSE.txt
	cp README-Windows.txt $LIBNFC_WINDOWS_DIR/

	# Remove Autotools Makefile.am
	find $LIBNFC_WINDOWS_DIR/ -name Makefile.am | xargs rm -f

	# Remove CMakeLists.txt
	find $LIBNFC_WINDOWS_DIR/ -name CMakeLists.txt | xargs rm -f

	# Build archive
	zip -r $LIBNFC_WINDOWS_ARCHIVE $LIBNFC_WINDOWS_DIR
	rm -rf $LIBNFC_WINDOWS_DIR
else
	echo "Windows archive is already done: skipped."
fi

