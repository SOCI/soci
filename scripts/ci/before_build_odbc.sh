#!/bin/bash -e
# Sets up environment for SOCI backend ODBC in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

psql --version
psql -c 'create database soci_test;' -U postgres
