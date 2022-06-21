#!/usr/bin/env bash
# Builds SOCI MySQL backend in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_MYSQL=ON \
    -DSOCI_MYSQL_TEST_CONNSTR:STRING="db=soci_test" \
    ..

run_make
