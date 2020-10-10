#!/bin/bash -e
# Builds and tests SOCI backend SQLite3 at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_POSTGRESQL=ON \
    -DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="dbname=soci_test user=postgres" \
    ..

run_make
run_test
