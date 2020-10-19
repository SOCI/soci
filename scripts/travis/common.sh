#!/bin/bash -e
# Common definitions used by SOCI build scripts at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
if [[ "$TRAVIS" != "true" ]] ; then
	echo "Running this script makes no sense outside of travis-ci.org"
	exit 1
fi
#
# Environment
#
TCI_NUMTHREADS=2
if [[ -f /sys/devices/system/cpu/online ]]; then
	# Calculates 1.5 times physical threads
	TCI_NUMTHREADS=$(( ( $(cut -f 2 -d '-' /sys/devices/system/cpu/online) + 1 ) * 15 / 10  ))
fi

# These options are used for all builds.
SOCI_COMMON_CMAKE_OPTIONS='
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_VERBOSE_MAKEFILE=ON
    -DSOCI_ENABLE_WERROR=ON
    -DSOCI_STATIC=OFF
    -DSOCI_TESTS=ON
'

# These options are defaults and used by most builds, but not Valgrind one.
SOCI_DEFAULT_CMAKE_OPTIONS='
    -DSOCI_ASAN=ON
    -DSOCI_DB2=OFF
    -DSOCI_EMPTY=OFF
    -DSOCI_FIREBIRD=OFF
    -DSOCI_MYSQL=OFF
    -DSOCI_ODBC=OFF
    -DSOCI_ORACLE=OFF
    -DSOCI_POSTGRESQL=OFF
    -DSOCI_SQLITE3=OFF
'

#
# Functions
#
tmstamp()
{
    echo -n "[$(date '+%H:%M:%S')]" ;
}

run_make()
{
    [ $TCI_NUMTHREADS -gt 0 ] && make -j $TCI_NUMTHREADS || make
}

run_test()
{
    ctest -V --output-on-failure "$@" .
}

run_test_memcheck()
{
    valgrind --leak-check=full --suppressions=${TRAVIS_BUILD_DIR}/valgrind.suppress --error-exitcode=1 --trace-children=yes ctest -V --output-on-failure "$@" .
}
