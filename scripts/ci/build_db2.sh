#!/bin/bash -e
# Builds SOCI DB2 backend in CI builds
#
# Copyright (c) 2013 Brian R. Toonen <toonen@alcf.anl.gov>
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

cmake ${SOCI_DEFAULT_CMAKE_OPTIONS} \
    -DSOCI_DB2=ON \
    -DSOCI_DB2_TEST_CONNSTR:STRING="DSN=SOCITEST\;Uid=db2inst1\;Pwd=db2inst1" \
    ..

run_make
