#!/bin/bash -e
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source $TRAVIS_BUILD_DIR/bin/ci/common.sh

cmake -DSOCI_STATIC=OFF \
    -DSOCI_TESTS=ON \
    -DSOCI_DB2=OFF \ 
    -DSOCI_EMPTY=ON \ 
    -DSOCI_FIREBIRD=OFF \ 
    -DSOCI_MYSQL=OFF \ 
    -DSOCI_ODBC=OFF \ 
    -DSOCI_ORACLE=OFF \ 
    -DSOCI_POSTGRESQL=OFF \
    -DSOCI_SQLITE3=OFF \
    $TRAVIS_BUILD_DIR/src

make -j $TCI_NUMTHREADS
ctest -V --output-on-failure .

