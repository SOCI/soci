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

set(ODBC_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc backend regression test")

add_soci_test(NAME odbc ARGUMENTS ${ODBC_TEST_CONNSTR})
target_sources(soci_odbc_test 
  PRIVATE 
    test-odbc-access.cpp
    test-odbc-db2.cpp
    test-odbc-mssql.cpp
    test-odbc-mysql.cpp
    test-odbc-postgresql.cpp
)