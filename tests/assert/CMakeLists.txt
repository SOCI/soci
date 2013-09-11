###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

set(SOCI_TESTS_COMMON common-tests.h)

###############################################################################
soci_backend_test(
  BACKEND Empty
  SOURCE test-empty.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "dummy")

###############################################################################
soci_backend_test(
  BACKEND DB2
  SOURCE test-db2.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off")

###############################################################################
soci_backend_test(
  BACKEND Firebird
  SOURCE test-firebird.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "service=/tmp/test.fdb user=SYSDBA password=masterkey")

###############################################################################
soci_backend_test(
  BACKEND MySQL
  SOURCE test-mysql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "db=soci_test")

###############################################################################
# ODBC
if (WIN32)
  
  # MDBTools driver seems unreliable
  soci_backend_test(
    NAME access
    BACKEND ODBC
    SOURCE test-odbc-access.cpp ${SOCI_TESTS_COMMON}
    CONNSTR "test-access.dsn")

  # We have no means to test SQL Server at travis-ci.org
  soci_backend_test(
    NAME mssql
    BACKEND ODBC
    SOURCE test-odbc-mssql.cpp ${SOCI_TESTS_COMMON}
    CONNSTR "test-mssql.dsn")
endif()

soci_backend_test(
  NAME mysql
  BACKEND ODBC
  SOURCE test-odbc-mysql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "test-mysql.dsn")

soci_backend_test(
  NAME postgresql
  BACKEND ODBC
  SOURCE test-odbc-postgresql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "test-postgresql.dsn")

###############################################################################
soci_backend_test(
  BACKEND Oracle
  SOURCE test-oracle.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "service=orcl user=scott password=tiger")

###############################################################################
soci_backend_test(
  BACKEND PostgreSQL
  SOURCE test-postgresql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "dbname=soci_test")

###############################################################################
soci_backend_test(
  BACKEND SQLite3
  SOURCE test-sqlite3.cpp ${SOCI_TESTS_COMMON}
  CONNSTR ":memory:")

get_target_property(X soci_empty COMPILE_DEFINITIONS)
