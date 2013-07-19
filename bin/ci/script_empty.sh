#!/bin/bash -e
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh

cmake \
    ${TRAVIS_BUILD_DIR}/src

make -j $TCI_NUMTHREADS
ctest -V --output-on-failure .

