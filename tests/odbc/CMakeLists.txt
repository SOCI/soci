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
include(AddSociTest)

set(SOCI_ODBC_ACCESS_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc access backend regression test")
set(SOCI_ODBC_DB2_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc db2 backend regression test")
set(SOCI_ODBC_MSSQL_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc mssql backend regression test")
set(SOCI_ODBC_MYSQL_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc mysql backend regression test")
set(SOCI_ODBC_POSTGRESQL_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc postgresql backend regression test")

add_soci_test(NAME odbc_access ARGUMENTS ${SOCI_ODBC_ACCESS_TEST_CONNSTR})
target_sources(soci_odbc_acces_test PRIVATE test-odbc-access.cpp)

add_soci_test(NAME odbc_db2 ARGUMENTS ${SOCI_ODBC_DB2_TEST_CONNSTR})
target_sources(soci_odbc_db2_test PRIVATE test-odbc-db2.cpp)

add_soci_test(NAME odbc_mssql ARGUMENTS ${SOCI_ODBC_MSSQL_TEST_CONNSTR})
target_sources(soci_odbc_mssql_test PRIVATE test-odbc-mssql.cpp)

add_soci_test(NAME odbc_mysql ARGUMENTS ${SOCI_ODBC_MYSQL_TEST_CONNSTR})
target_sources(soci_odbc_mysql_test PRIVATE test-odbc-mysql.cpp)

add_soci_test(NAME odbc_postgresql ARGUMENTS ${SOCI_ODBC_POSTGRESQL_TEST_CONNSTR})
target_sources(soci_odbc_postgresql_test PRIVATE test-odbc-postgresql.cpp)