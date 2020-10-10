#!/bin/bash -e
# Builds and tests SOCI backend Oracle at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh
source ${TRAVIS_BUILD_DIR}/scripts/travis/oracle.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DWITH_BOOST=OFF \
    -DSOCI_ORACLE=ON \
    -DSOCI_ORACLE_TEST_CONNSTR:STRING="service=XE user=travis password=travis" \
    ..

run_make
run_test
