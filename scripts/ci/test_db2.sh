#!/bin/bash -e
# Tests SOCI DB2 backend in CI builds
#
# Copyright (c) 2013 Brian R. Toonen <toonen@alcf.anl.gov>
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

LSAN_OPTIONS=suppressions=${SOCI_SOURCE_DIR}/scripts/suppress_db2.txt run_test
