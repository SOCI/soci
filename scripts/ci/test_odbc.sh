#!/bin/bash -e
# Tests SOCI ODBC backend in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# Exclude the tests which can't be run due to the absence of ODBC drivers (MS
# SQL and MySQL).
run_test -E 'soci_odbc_test_m.sql'
