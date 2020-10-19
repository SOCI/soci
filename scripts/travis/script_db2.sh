#!/bin/bash -e
# Builds and tests SOCI backend DB2 at travis-ci.org
#
# Copyright (c) 2013 Brian R. Toonen <toonen@alcf.anl.gov>
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_DB2=ON \
    -DSOCI_DB2_TEST_CONNSTR:STRING="DSN=SOCITEST\;Uid=db2inst1\;Pwd=db2inst1" \
    ..

run_make
LSAN_OPTIONS=suppressions=${TRAVIS_BUILD_DIR}/scripts/suppress_db2.txt run_test
