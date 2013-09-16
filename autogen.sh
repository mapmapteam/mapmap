#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

which gnome-autogen.sh || {
    echo "You need to install gnome-common"
    exit 1
}

ACLOCAL_FLAGS="$ACLOCAL_FLAGS" USE_GNOME2_MACROS=1 . gnome-autogen.sh
