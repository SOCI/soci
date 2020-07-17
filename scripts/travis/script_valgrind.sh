#!/bin/bash -e
# Builds and tests SOCI at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2015 Sergei Nikulov <sergey.nikulov@gmail.com>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

cmake \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSOCI_BUILD_TESTING=ON \
    -DSOCI_WITH_DB2=ON \
    -DSOCI_WITH_FIREBIRD=ON \
    -DSOCI_WITH_MYSQL=ON \
    -DSOCI_WITH_ORACLE=ON \
    -DSOCI_WITH_POSTGRESQL=ON \
    -DSOCI_WITH_SQLITE3=ON \
    ..

run_make
run_test_memcheck
