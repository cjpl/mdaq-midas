#!/bin/bash
## Cleanup all autoconf files

#WK_DIR=$(dirname $(readlink -f $0))
AUTO_FILES="config.* configure depcomp install-sh ltmain.sh Makefile.in missing"

echo "Clean up autoconf generated files ..."
rm -f ${AUTO_FILES}

echo "Add libtool supporting (libtoolize) ..."
libtoolize --automake --copy

echo "Generating \"aclocal.m4\" (aclocal) for autoconf ..."
aclocal

echo "Generating script \"configure\" (autoconf) ..."
autoconf

echo "Generating \"Makefile.in\" (automake) ..."
automake --add-missing --copy

echo "Clean up autoconf m4 cache ..."
rm -rf autom4te.cache

