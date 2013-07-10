#!/bin/bash
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh
echo $TRAVIS_BUILD_DIR
echo $PWD
cmake \
    -DSOCI_STATIC=OFF \
    -DSOCI_TESTS=ON \
    -DSOCI_EMPTY=ON \ 
    -DSOCI_EMPTY_TEST_CONNSTR:STRING="dummy connection" \
    -DSOCI_DB2=OFF \ 
    -DSOCI_FIREBIRD=OFF \ 
    -DSOCI_MYSQL=OFF \ 
    -DSOCI_ODBC=OFF \ 
    -DSOCI_ORACLE=OFF \ 
    -DSOCI_POSTGRESQL=OFF \ 
    -DSOCI_SQLITE3=OFF ..

[[ ${NUMTHREADS} -gt 0 ]] && make -j ${NUMTHREADS} || make
ctest -V --output-on-failure .

