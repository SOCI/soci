#!/bin/bash -e
# Sets up environment for SOCI backend ODBC in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

psql --version
psql -c 'create database soci_test;' -U postgres

# Test connection to the database via ODBC.
#
# Note that using FILEDSN requires -k (Use SQLDriverConnect) and doesn't work
# without it.
echo 'select 2+2;' | isql -b -k -v "FILEDSN=${SOCI_SOURCE_DIR}/tests/odbc/test-postgresql.dsn"
