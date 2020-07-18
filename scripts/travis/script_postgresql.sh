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
    -DBUILD_TESTING=ON \
    -DSOCI_STATIC=OFF \
    -DSOCI_WITH_EMPTY=OFF \
    -DSOCI_WITH_POSTGRESQL=ON \
    -DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="dbname=soci_test user=postgres" \
    ..

run_make
run_test
