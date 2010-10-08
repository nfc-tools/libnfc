#! /bin/sh

# Stop script on first error.
set -e

# Retrieve libnfc version from configure.ac
LIBNFC_VERSION=$(grep AC_INIT configure.ac | sed 's/^.*(\(.*\))/\1/g' | awk -F', ' '{ print $2 }')

# Easiest part: GNU/linux, BSD and other POSIX systems.
LIBNFC_AUTOTOOLS_ARCHIVE=libnfc-$LIBNFC_VERSION.tar.gz

echo ">>> Cleaning sources..."
# First, clean what we can
rm -f configure config.h config.h.in
autoreconf -is && ./configure && make distclean
svn propget svn:ignore . | xargs rm -rf
echo "<<< Sources cleaned."

if [ ! -f $LIBNFC_AUTOTOOLS_ARCHIVE ]; then
	echo ">>> Autotooled archive generation..."

	# Second, generate dist archive (and test it)
	autoreconf -is && ./configure && make distcheck

	# Finally, clean up
	make distclean
	echo "<<< Autotooled archive generated."
else
	echo "--- Autotooled archive (GNU/Linux, BSD, etc.) is already done: skipped."
fi

# Documentation part
LIBNFC_DOC_DIR=libnfc-doc-$LIBNFC_VERSION
LIBNFC_DOC_ARCHIVE=$LIBNFC_DOC_DIR.zip

if [ ! -f $LIBNFC_DOC_ARCHIVE ]; then
	echo ">>> Documentation archive generation..."
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
	echo "<<< Documentation archive generated."
else
	echo "--- Documentation archive is already done: skipped."
fi

