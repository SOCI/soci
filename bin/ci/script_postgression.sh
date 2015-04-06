#!/bin/bash -e
# Builds and tests SOCI backend SQLite3 at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh

# Get the database URI to use and mangle it into the form that both our tests
# and psql understand.
SOCI_POSTGRESQL_CONNSTR=$(curl http://api.postgression.com | \
sed 's|postgres://\([^:]\+\):\([^@]\+\)@\([^:]\+\):\([0-9]\+\)/\(.*\)|user=\1 password=\2 host=\3 port=\4 dbname=\5|')

echo "Postgression connection parameters: $SOCI_POSTGRESQL_CONNSTR"

echo "PostgreSQL client and server versions:"
psql --version
psql -c 'select version();' "$SOCI_POSTGRESQL_CONNSTR"

cmake \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DSOCI_TESTS=ON \
    -DSOCI_STATIC=OFF \
    -DSOCI_DB2=OFF \
    -DSOCI_EMPTY=OFF \
    -DSOCI_FIREBIRD=OFF \
    -DSOCI_MYSQL=OFF \
    -DSOCI_ODBC=OFF \
    -DSOCI_ORACLE=OFF \
    -DSOCI_POSTGRESQL=ON \
    -DSOCI_SQLITE3=OFF \
    -DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="$SOCI_POSTGRESQL_CONNSTR" \
    ..

run_make
run_test
