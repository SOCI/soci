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
    SOURCE test-odbc-access.cpp ${SOCI_TESTS_COMMON}
    CONNSTR "test-access.dsn")

  # We have no means to test SQL Server at travis-ci.org
  soci_backend_test(
    NAME mssql
    BACKEND ODBC
    SOURCE test-odbc-mssql.cpp ${SOCI_TESTS_COMMON}
    CONNSTR "test-mssql.dsn")
else()
    message(STATUS "SQL Server, MS Access tests disabled on non-Windows platform")
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
