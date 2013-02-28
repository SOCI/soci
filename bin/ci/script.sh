#!/bin/bash
# Run script actions for SOCI build at travis-ci.org
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ./bin/ci/common.sh
# Build SOCI using CMake (primary build configuration)
mkdir -p src/_build
cd src/_build
echo "$(tmstamp) *** script::cmake-config starting $(date) ***"
cmake \
	-DSOCI_TESTS=ON \
	-DSOCI_EMPTY_TEST_CONNSTR:STRING="dummy connection" \
	-DSOCI_FIREBIRD_TEST_CONNSTR:STRING="service=LOCALHOST:/tmp/soci_test.fdb user=SYSDBA password=masterkey" \
	-DSOCI_MYSQL_TEST_CONNSTR:STRING="db=soci_test" \
	-DSOCI_ORACLE_TEST_CONNSTR:STRING="service=brzuchol.loskot.net user=soci_tester password=soci_secret" \
	-DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="dbname=soci_test user=postgres" \
	-DSOCI_SQLITE3_TEST_CONNSTR:STRING="soci_test.db" \
	-DSOCI_ODBC_TEST_POSTGRESQL_CONNSTR="FILEDSN=${PWD}/../backends/odbc/test/test-postgresql.dsn;" \
	-DSOCI_ODBC_TEST_MYSQL_CONNSTR="FILEDSN=${PWD}/../backends/odbc/test/test-mysql.dsn;" \
	..
echo "$(tmstamp) *** script::cmake-config finished $(date) ***"

echo "$(tmstamp) *** script::cmake-build starting $(date) ***"
cmake --build .
echo "$(tmstamp) *** script::cmake-build finished $(date) ***"

echo "$(tmstamp) *** script::cmake-test starting $(date) ***"
ctest -V --output-on-failure .
echo "$(tmstamp) *** script::cmake-test finished $(date) ***"
