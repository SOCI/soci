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
include(AddSociTest)

# Request CATCH to disable all the C++11 features
if (NOT SOCI_CXX11)
  add_definitions(-DCATCH_CONFIG_NO_CPP11)
endif()

if(WITH_DB2)
  add_soci_test(NAME odbc)
  add_subdirectory(db2)
endif()

if(WITH_EMPTY)
  add_soci_test(NAME empty)
  add_subdirectory(empty)
endif()

if(WITH_FIREBIRD)
  add_soci_test(NAME firebird)
  add_subdirectory(firebird)
endif()

if(WITH_MYSQL)
  add_soci_test(NAME mysql)
  add_subdirectory(mysql)
endif()

if(WITH_ODBC)
  add_soci_test(NAME odbc)
  add_subdirectory(odbc)
endif()

if(WITH_ORACLE)
  add_soci_test(NAME oracle)
  add_subdirectory(oracle)
endif()

if(WITH_POSTGRESQL)
  add_soci_test(NAME postgresql)
  add_subdirectory(postgresql)
endif()

if(WITH_SQLITE3)
  add_soci_test(NAME sqlite3)
  add_subdirectory(sqlite3)
endif()
