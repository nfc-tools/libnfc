#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4 ltmain.sh

touch README

LIBTOOLIZE=libtoolize
case `uname -s` in
Darwin)
  LIBTOOLIZE=glibtoolize
  ACLOCAL_ARGS="-I /opt/local/share/aclocal/"
  #  SKIP_PCSC="1"
  ;;
FreeBSD)
  ACLOCAL_ARGS="-I /usr/local/share/aclocal/"
  ;;
esac

ACLOCAL_TITLE=`aclocal --version | head -n 1`
AUTOHEADER_TITLE=`autoheader --version | head -n 1`
AUTOCONF_TITLE=`autoconf --version | head -n 1`
LIBTOOLIZE_TITLE=`$LIBTOOLIZE --version | head -n 1`
AUTOMAKE_TITLE=`automake --version | head -n 1`

echo "Running $ACLOCAL_TITLE..." ; aclocal $ACLOCAL_ARGS || exit 1
echo "Running $AUTOHEADER_TITLE..." ; autoheader || exit 1
echo "Running $AUTOCONF_TITLE..." ; autoconf || exit 1
echo "Running $LIBTOOLIZE_TITLE..." ; $LIBTOOLIZE --copy --automake || exit 1
echo "Running $AUTOMAKE_TITLE..." ; automake --add-missing --copy --gnu || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
