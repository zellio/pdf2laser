#!/usr/bin/env sh

set -x

aclocal -I m4 --install
autoheader
automake --foreign --add-missing
autoconf
