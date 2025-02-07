#!/usr/bin/env bash
# Builds SOCI "empty" backend in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

run_cmake_for_empty()
{
    cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD:-11} \
        -DSOCI_EMPTY=ON \
        ..
}

build_example()
{
  cmake -S "../examples/$1" -B "$1"
  cmake --build "$1"
}

run_cmake_for_empty
run_make

if [[ "$BUILD_EXAMPLES" == "YES" ]]; then
  # This example simulates SOCI sources being embedded in the project dir
  build_example subdir-include

  # Install previously built SOCI library on the system
  sudo make install

  # This example simulates SOCI being installed on the target system
  build_example connect
fi

# Test release branch packaging and building from the package
if [[ "$TEST_RELEASE_PACKAGE" == "YES" ]] && [[ "$SOCI_CI_BRANCH" =~ ^release/[3-9]\.[0-9]$ ]]; then
    ME=`basename "$0"`

    run_apt update
    run_apt install python3-venv

    SOCI_VERSION=$(cat "$SOCI_SOURCE_DIR/include/soci/version.h" | grep -Po "(.*#define\s+SOCI_LIB_VERSION\s+.+)\K([3-9]_[0-9]_[0-9])" | sed "s/_/\./g")
    if [[ ! "$SOCI_VERSION" =~ ^[4-9]\.[0-9]\.[0-9]$ ]]; then
        echo "${ME} ERROR: Invalid format of SOCI version '$SOCI_VERSION'. Aborting."
        exit 1
    else
        echo "${ME} INFO: Creating source package 'soci-${SOCI_VERSION}.tar.gz' from '$SOCI_CI_BRANCH' branch"
    fi

    cd $SOCI_SOURCE_DIR
    $SOCI_SOURCE_DIR/scripts/release.sh --use-local-branch $SOCI_CI_BRANCH

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
