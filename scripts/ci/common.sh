#!/bin/sh
# Common definitions used by SOCI build scripts in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Note that this is a /bin/sh script because it is used from install.sh
# which installs bash under FreeBSD and so we can't rely on bash being
# available yet.

# Stop on all errors.
set -e

if [ "$SOCI_CI" != "true" ] ; then
	echo "Running this script is only useful in the CI builds"
	exit 1
fi

if [ -n "$RUNNER_DEBUG" ]; then
  set -x
fi

backend_settings=${SOCI_SOURCE_DIR}/scripts/ci/${SOCI_CI_BACKEND}.sh
if [ -f ${backend_settings} ]; then
    . ${backend_settings}
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

# Directory where the build happens.
#
# Note that the existing commands suppose that the build directory is an
# immediate subdirectory of the source one, so don't change this.
builddir="${SOCI_SOURCE_DIR}/_build"

# These options are used for all builds.
SOCI_COMMON_CMAKE_OPTIONS="
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_VERBOSE_MAKEFILE=ON
    -DSOCI_ENABLE_WERROR=ON
    -DSOCI_TESTS=ON
    -DCMAKE_UNITY_BUILD=$SOCI_ENABLE_UNITY_BUILD
"

if [ -n "${SOCI_CXXSTD}" ]; then
    SOCI_COMMON_CMAKE_OPTIONS="$SOCI_COMMON_CMAKE_OPTIONS -DCMAKE_CXX_STANDARD=${SOCI_CXXSTD}"
fi

if [ -n "${WITH_BOOST}" ]; then
    SOCI_COMMON_CMAKE_OPTIONS="$SOCI_COMMON_CMAKE_OPTIONS -DWITH_BOOST=${WITH_BOOST}"
else
    # Build Boost bindings by default
    SOCI_COMMON_CMAKE_OPTIONS="$SOCI_COMMON_CMAKE_OPTIONS -DWITH_BOOST=ON"
fi

if [ -n "${SOCI_BUILD_STATIC}" ]; then
    SOCI_COMMON_CMAKE_OPTIONS="${SOCI_COMMON_CMAKE_OPTIONS} -DSOCI_SHARED=OFF"
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

# Most builds also enable UBSAN but it has to be disabled for some of them.
if [ -z "${SOCI_NO_UBSAN}" ]; then
    SOCI_DEFAULT_CMAKE_OPTIONS="$SOCI_DEFAULT_CMAKE_OPTIONS -DSOCI_UBSAN=ON"
fi

#
# Functions
#
tmstamp()
{
    echo -n "[$(date '+%H:%M:%S')]" ;
}

SOCI_APT_OPTIONS='-q -y -o=Dpkg::Use-Pty=0 --no-install-recommends'

run_apt()
{
    # Disable some (but not all) output.
    sudo apt-get $SOCI_APT_OPTIONS "$@"
}

run_make()
{
    make -j $num_cpus
}

run_test()
{
    # The example project doesn't have any tests, but otherwise their absence
    # is an error and means that something has gone wrong.
    if [ "$BUILD_EXAMPLES" == "YES" ]; then
        no_tests_action=ignore
    else
        no_tests_action=error
    fi
    ctest -V --output-on-failure --no-tests=${no_tests_action} "$@" .
}
