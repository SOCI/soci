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

if (WIN32)
  # MDBTools driver seems unreliable
  soci_backend_test(
    NAME access
    BACKEND ODBC
    DEPENDS ODBC
    SOURCE test-odbc-access.cpp ${SOCI_TESTS_COMMON}
    CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-access.dsn")
else()
    message(STATUS "MS Access test disabled on non-Windows platform")
endif()

set(MSSQL_VER $ENV{MSSQL_VER})
if (NOT MSSQL_VER)
    # Use the same value that was used before by default.
    set(MSSQL_VER "2014")
endif()
configure_file("test-mssql.dsn.in" "test-mssql.dsn" @ONLY)

soci_backend_test(
  NAME mssql
  BACKEND ODBC
  DEPENDS ODBC
  SOURCE test-odbc-mssql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "FILEDSN=${CMAKE_CURRENT_BINARY_DIR}/test-mssql.dsn")

soci_backend_test(
  NAME mysql
  BACKEND ODBC
  DEPENDS ODBC
  SOURCE test-odbc-mysql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-mysql.dsn")

if(WIN32)
  set(TEST_PGSQL_DSN "test-postgresql-win64.dsn")
else()
  set(TEST_PGSQL_DSN "test-postgresql.dsn")
endif()
soci_backend_test(
  NAME postgresql
  BACKEND ODBC
  DEPENDS ODBC
  SOURCE test-odbc-postgresql.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_PGSQL_DSN}")

# TODO: DB2 backend is tested by Travis CI on dedicated VM, separate from ODBC,
# in order to test DB2 with ODBC, it would be best to install DB2 driver only.
# if (NOT $ENV{TRAVIS})
option(WITH_ODBC_TEST_DB2 "Build ODBC DB2 test" OFF)
if (WITH_ODBC_TEST_DB2)
  soci_backend_test(
    NAME db2
    BACKEND ODBC
    SOURCE test-odbc-db2.cpp
    CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-db2.dsn")
else()
  message(STATUS "ODBC DB2 test disabled.")
endif()
