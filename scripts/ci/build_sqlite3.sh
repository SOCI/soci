#!/bin/bash -e
# Builds SOCI SQLite3 backend in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_SQLITE3=ON \
    -DSOCI_SQLITE3_TEST_CONNSTR:STRING="soci_test.db" \
    ..

run_make
