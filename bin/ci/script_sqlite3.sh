#!/bin/bash
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh

builddir=${TRAVIS_BUILD_DIR}/src/_build
cd ${builddir} || { echo "'${builddir}' does not exist"; exit 1; }

cmake \
	-DSOCI_STATIC=OFF \
	-DSOCI_TESTS=ON \
    -DSOCI_SQLITE3=ON \
    -DSOCI_SQLITE3_TEST_CONNSTR:STRING="soci_test.db" \
    -DSOCI_DB2=OFF \ 
    -DSOCI_EMPTY=OFF \ 
    -DSOCI_FIREBIRD=OFF \ 
    -DSOCI_MYSQL=OFF \ 
    -DSOCI_ODBC=OFF \ 
    -DSOCI_ORACLE=OFF \ 
    -DSOCI_POSTGRESQL=OFF \ 
	..

[[ ${NUMTHREADS} -gt 0 ]] && make -j ${NUMTHREADS} || make
ctest -V --output-on-failure .

