#!/bin/bash -e
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh

mkdir -p $TCI_BLD_DIR
cd $TCI_BLD_DIR

cmake \
    -DSOCI_STATIC=OFF \
    -DSOCI_TESTS=ON \
    -DSOCI_DB2=OFF \ 
    -DSOCI_EMPTY=ON \ 
    -DSOCI_EMPTY_TEST_CONNSTR:STRING="dummy.db" \
    -DSOCI_FIREBIRD=OFF \ 
    -DSOCI_MYSQL=OFF \ 
    -DSOCI_ODBC=OFF \ 
    -DSOCI_ORACLE=OFF \ 
    -DSOCI_POSTGRESQL=OFF \
    -DSOCI_SQLITE3=OFF \
    $TCI_SRC_DIR

make -j $TCI_NUMTHREADS
ctest -V --output-on-failure .

