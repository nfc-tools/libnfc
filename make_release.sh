#! /bin/sh

# Stop script on first error.
set -e

# Retrieve libnfc version from configure.ac
LIBNFC_VERSION=$(grep AC_INIT configure.ac | sed 's/^.*(\(.*\))/\1/g' | awk -F', ' '{ print $2 }')

## Easiest part: GNU/linux, BSD and other POSIX systems.
LIBNFC_AUTOTOOLS_ARCHIVE=libnfc-$LIBNFC_VERSION.tar.gz

if [ ! -f $LIBNFC_AUTOTOOLS_ARCHIVE ]; then
	# First, we can test archive using "distcheck"
	./autogen.sh && make distcheck && make distclean

	# We are ready to make a good autotools release.
	./autogen.sh && make dist
else
	echo "Autotooled archive (GNU/Linux, BSD, etc.) is already done: skipped."
fi

# Windows part
LIBNFC_WINDOWS_DIR=libnfc-$LIBNFC_VERSION-windows
LIBNFC_WINDOWS_ARCHIVE=$LIBNFC_WINDOWS_DIR.zip

if [ ! -f $LIBNFC_WINDOWS_ARCHIVE ]; then
	if [ -d $LIBNFC_WINDOWS_DIR ]; then
		rm -rf $LIBNFC_WINDOWS_DIR
	fi

	mkdir -p $LIBNFC_WINDOWS_DIR

	# Copy sources
	cp src/*.c $LIBNFC_WINDOWS_DIR/
	cp src/*.h $LIBNFC_WINDOWS_DIR/

	# Copy important files
	cp LICENSE $LIBNFC_WINDOWS_DIR/
	cp README $LIBNFC_WINDOWS_DIR/

	# Copy Visual C++ project files
	cp -r win32 $LIBNFC_WINDOWS_DIR/vc-project

	# Remove svn stuff
	find $LIBNFC_WINDOWS_DIR -name ".svn" -type d | xargs rm -rf 

	# Build archive
	zip -r $LIBNFC_WINDOWS_ARCHIVE $LIBNFC_WINDOWS_DIR
	rm -rf $LIBNFC_WINDOWS_DIR
else
	echo "Windows archive is already done: skipped."
fi

