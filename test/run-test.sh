#!/bin/sh

export BASE_DIR="`dirname $0`"

if test -z "$NO_MAKE"; then
    make -C "$BASE_DIR/../" > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C "$BASE_DIR" echo-cutter`"
fi

"$CUTTER" --keep-opening-modules -s "$BASE_DIR" "$@" "$BASE_DIR"
#         ^^^^^^^^^^^^^^^^^^^^^^
# FIXME: Remove this workaround once cutter has been fixed upstream.
# Bug report:
# http://sourceforge.net/mailarchive/forum.php?thread_name=20100626123941.GA258%40blogreen.org&forum_name=cutter-users-en
