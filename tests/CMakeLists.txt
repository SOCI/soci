###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

###############################################################################
# Test Setup
###############################################################################
set(DB2_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the db2 backend regression test")
set(EMPTY_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the empty backend regression test")
set(FIREBIRD_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the firebird backend regression test")
set(MYSQL_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the mysql backend regression test")
set(ODBC_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the odbc backend regression test")
set(ORACLE_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the orcacle backend regression test")
set(POSTGRESQL_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the postgresql backend regression test")
set(SQLITE3_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the sqlite3 backend regression test")

###############################################################################
# Test Dependencies
###############################################################################
add_library(Catch::Catch IMPORTED INTERFACE)
target_include_directories(Catch::Catch INTERFACE ${PROJECT_SOURCE_DIR}/extern/catch)

# Request CATCH to disable all the C++11 features
if (NOT SOCI_CXX11)
  target_compile_definitions(Catch::Catch INTERFACE CATCH_CONFIG_NO_CPP11)
endif()

###############################################################################
# Test Targets
###############################################################################
include(AddSociTest)

if(WITH_DB2)
  add_soci_test(NAME db2 ARGUMENTS ${DB2_TEST_CONNSTR})
  add_subdirectory(db2)
endif()

if(WITH_EMPTY)
  add_soci_test(NAME empty ARGUMENTS ${EMPTY_TEST_CONNSTR})
  add_subdirectory(empty)
endif()

if(WITH_FIREBIRD)
  add_soci_test(NAME firebird ARGUMENTS ${FIREBIRD_TEST_CONNSTR})
  add_subdirectory(firebird)
endif()

if(WITH_MYSQL)
  add_soci_test(NAME mysql ARGUMENTS ${MYSQL_TEST_CONNSTR})
  add_subdirectory(mysql)
endif()

if(WITH_ODBC)
  add_soci_test(NAME odbc ARGUMENTS ${ODBC_TEST_CONNSTR})
  add_subdirectory(odbc)
endif()

if(WITH_ORACLE)
  add_soci_test(NAME oracle ARGUMENTS ${ORACLE_TEST_CONNSTR})
  add_subdirectory(oracle)
endif()

if(WITH_POSTGRESQL)
  add_soci_test(NAME postgresql ARGUMENTS ${POSTGRESQL_TEST_CONNSTR})
  add_subdirectory(postgresql)
endif()

if(WITH_SQLITE3)
  add_soci_test(NAME sqlite3 ARGUMENTS ${SQLITE3_TEST_CONNSTR})
  add_subdirectory(sqlite3)
endif()
