#!/bin/bash -e
# Builds and tests SOCI backend SQLite3 at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSOCI_ASAN=ON \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DSOCI_BUILD_TESTING=ON \
    -DSOCI_WITH_EMPTY=OFF \
    -DSOCI_WITH_MYSQL=ON \
    -DSOCI_MYSQL_TEST_CONNSTR:STRING="db=soci_test" \
    ..

run_make
run_test
