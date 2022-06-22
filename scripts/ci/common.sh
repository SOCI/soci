#!/usr/bin/env bash
# Common definitions used by SOCI build scripts in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#

# Stop on all errors.
set -e

if [[ "$SOCI_CI" != "true" ]] ; then
	echo "Running this script is only useful in the CI builds"
	exit 1
fi

backend_settings=${SOCI_SOURCE_DIR}/scripts/ci/${SOCI_CI_BACKEND}.sh
if [ -f ${backend_settings} ]; then
    source ${backend_settings}
fi

#
# Environment
#
case `uname` in
    Linux)
        num_cpus=`nproc`
        ;;

    Darwin | FreeBSD)
        num_cpus=`sysctl -n hw.ncpu`
        ;;

    *)
        num_cpus=1
esac

if [[ ${num_cpus} != 1 ]]; then
    ((num_cpus++))
fi

# Directory where the build happens.
#
# Note that the existing commands suppose that the build directory is an
# immediate subdirectory of the source one, so don't change this.
builddir="${SOCI_SOURCE_DIR}/_build"

# These options are used for all builds.
SOCI_COMMON_CMAKE_OPTIONS='
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_VERBOSE_MAKEFILE=ON
    -DSOCI_ENABLE_WERROR=ON
    -DSOCI_STATIC=OFF
    -DSOCI_TESTS=ON
'

if [[ -n ${WITH_BOOST} ]]; then
    SOCI_COMMON_CMAKE_OPTIONS="$SOCI_COMMON_CMAKE_OPTIONS -DWITH_BOOST=${WITH_BOOST}"
fi

if [[ -n ${WITHOUT_VISIBILITY} ]]; then
    SOCI_COMMON_CMAKE_OPTIONS="$SOCI_COMMON_CMAKE_OPTIONS -DSOCI_VISIBILITY=OFF"
fi

# These options are defaults and used by most builds, but not Valgrind one.
SOCI_DEFAULT_CMAKE_OPTIONS="${SOCI_COMMON_CMAKE_OPTIONS}
    -DSOCI_ASAN=ON
    -DSOCI_DB2=OFF
    -DSOCI_EMPTY=OFF
    -DSOCI_FIREBIRD=OFF
    -DSOCI_MYSQL=OFF
    -DSOCI_ODBC=OFF
    -DSOCI_ORACLE=OFF
    -DSOCI_POSTGRESQL=OFF
    -DSOCI_SQLITE3=OFF
"

#
# Functions
#
tmstamp()
{
    echo -n "[$(date '+%H:%M:%S')]" ;
}

run_make()
{
    make -j $num_cpus
}

run_test()
{
    ctest -V --output-on-failure "$@" .
}
