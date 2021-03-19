#!/bin/bash -e
# Sets up environment for SOCI backend ODBC in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# Create PostgreSQL database we use for ODBC tests too.
${SOCI_SOURCE_DIR}/scripts/ci/before_build_postgresql.sh

# Test connection to the database via ODBC.
#
# Note that using FILEDSN requires -k (Use SQLDriverConnect) and doesn't work
# without it.
echo 'select 2+2;' | isql -b -k -v "FILEDSN=${SOCI_SOURCE_DIR}/tests/odbc/test-postgresql.dsn"
