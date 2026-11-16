#!/usr/bin/env bash
# Builds SOCI DB2 backend in CI builds
#
# Copyright (c) 2013 Brian R. Toonen <toonen@alcf.anl.gov>
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# Semicolons in the connection string must be escaped for CMake.
cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_DB2=ON \
    -DSOCI_DB2_TEST_CONNSTR:STRING="$(db2_connstr | sed 's/;/\;/g')" \
    ..

run_make
