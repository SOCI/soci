#!/bin/bash -e
# Builds and tests SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

run_cmake_for_empty()
{
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
        -DSOCI_ASAN=ON \
        -DSOCI_TESTS=ON \
        -DSOCI_STATIC=OFF \
        -DSOCI_DB2=OFF \
        -DSOCI_EMPTY=ON \
        -DSOCI_FIREBIRD=OFF \
        -DSOCI_MYSQL=OFF \
        -DSOCI_ODBC=OFF \
        -DSOCI_ORACLE=OFF \
        -DSOCI_POSTGRESQL=OFF \
        -DSOCI_SQLITE3=OFF \
        ..
}

run_cmake_for_empty
run_make
run_test

# Test release branch packaging and building from the package
if [[ "$TEST_RELEASE_PACKAGE" == "YES" ]] && [[ "$TRAVIS_BRANCH" =~ ^release/[3-9]\.[0-9]$ ]]; then
    ME=`basename "$0"`

    sudo apt-get update -qq -y
    sudo apt-get install -qq -y python3.4-venv

    SOCI_VERSION=$(cat "$TRAVIS_BUILD_DIR/include/soci/version.h" | grep -Po "(.*#define\s+SOCI_LIB_VERSION\s+.+)\K([3-9]_[0-9]_[0-9])" | sed "s/_/\./g")
    if [[ ! "$SOCI_VERSION" =~ ^[4-9]\.[0-9]\.[0-9]$ ]]; then
        echo "${ME} ERROR: Invalid format of SOCI version '$SOCI_VERSION'. Aborting."
        exit 1
    else
        echo "${ME} INFO: Creating source package 'soci-${SOCI_VERSION}.tar.gz' from '$TRAVIS_BRANCH' branch"
    fi

    cd $TRAVIS_BUILD_DIR
    $TRAVIS_BUILD_DIR/scripts/release.sh --use-local-branch $TRAVIS_BRANCH

    if [[ ! -f "soci-${SOCI_VERSION}.tar.gz" ]]; then
        echo "${ME} ERROR: Archive file 'soci-${SOCI_VERSION}.tar.gz' not found. Aborting."
        exit 1
    fi

    echo "${ME} INFO: Unpacking source package 'soci-${SOCI_VERSION}.tar.gz'"
    tar -xzf soci-${SOCI_VERSION}.tar.gz

    echo "${ME} INFO: Building SOCI from source package 'soci-${SOCI_VERSION}.tar.gz'"
    cd soci-${SOCI_VERSION}
    mkdir _build
    echo $PWD

    run_cmake_for_empty
    run_make
    run_test
fi
