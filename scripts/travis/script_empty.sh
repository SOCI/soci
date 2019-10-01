#!/bin/bash -e
# Builds and tests SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

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

run_make
run_test
