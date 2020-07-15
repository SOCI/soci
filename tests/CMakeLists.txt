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
if(SOCI_WITH_DB2)
  add_subdirectory(db2)
endif()

if(SOCI_WITH_EMPTY)
  add_subdirectory(empty)
endif()

if(SOCI_WITH_FIREBIRD)
  add_subdirectory(firebird)
endif()

if(SOCI_WITH_MYSQL)
  add_subdirectory(mysql)
endif()

if(SOCI_WITH_ODBC)
  add_subdirectory(odbc)
endif()

if(SOCI_WITH_ORACLE)
  add_subdirectory(oracle)
endif()

if(SOCI_WITH_POSTGRESQL)
  add_subdirectory(postgresql)
endif()

if(SOCI_WITH_SQLITE3)
  add_subdirectory(sqlite3)
endif()
