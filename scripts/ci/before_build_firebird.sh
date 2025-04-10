#!/usr/bin/env bash
# Configure Firebird database for SOCI build in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, https://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

isql-fb -z -q -i /dev/null # --version
echo 'CREATE DATABASE "LOCALHOST:/tmp/soci_test.fdb" PAGE_SIZE = 16384;' > /tmp/create_soci_test.sql
isql-fb -u SYSDBA -p masterkey -i /tmp/create_soci_test.sql -q
cat /tmp/create_soci_test.sql
