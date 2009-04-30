#!/bin/sh
# $Id: autogen.sh,v 1.3 2008/05/28 20:05:17 mloskot Exp $
#
# Run this to generate all the initial makefiles, etc.
# This was lifted from the Gimp, and adapted slightly
# by Raph Levien
#
# Customized for C++/Tk by Mateusz Loskot <mateusz@loskot.net>
#
DIE=0

PROJECT="SOCI"

for libtoolize in glibtoolize libtoolize; do
    LIBTOOLIZE=`which $libtoolize 2>/dev/null`
    if test -x "$LIBTOOLIZE"; then
        break;
    fi
done

if test -d "autom4te.cache"; then
    rm -rf autom4te.cache
fi

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo "Error:"
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Get latest version from ftp://ftp.gnu.org/pub/gnu/autoconf/"
	DIE=1
}

($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
	echo "Error:"
	echo "You must have libtool installed to compile $PROJECT."
	echo "Get latest version from ftp://ftp.gnu.org/pub/gnu/libtool/"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo "Error:"
	echo "You must have automake installed to compile $PROJECT."
	echo "Get latest version from ftp://ftp.gnu.org/pub/gnu/automake/"
	DIE=1
}

(aclocal --version) < /dev/null > /dev/null 2>&1 || {
	echo "Error:"
	echo "You must have aclocal installed to compile $PROJECT."
    echo "aclocal is installed together with automake, so"
	echo "get latest version from ftp://ftp.gnu.org/pub/gnu/automake/"
	DIE=1
}
if test "$DIE" -eq 1; then
	exit 1
fi

echo "Running aclocal..."
aclocal -I ../build/m4
echo "Running libtoolize..."
$LIBTOOLIZE --force --copy
echo "Running automake..."
automake --add-missing --copy
echo "Running autoconf..."
autoconf

echo
echo "Now type './configure' to generate Makefile files for $PROJECT."
echo "Run './configure --help' to get list of possible options."

