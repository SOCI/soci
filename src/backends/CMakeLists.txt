###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010 Mateusz Loskot
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
if(WITH_DB2)
  add_subdirectory(db2)
endif()

if(WITH_EMPTY)
  add_subdirectory(empty)
endif()

if(WITH_FIREBIRD)
  add_subdirectory(firebird)
endif()

if(WITH_MYSQL)
  add_subdirectory(mysql)
endif()

if(WITH_ODBC)
  add_subdirectory(odbc)
endif()

if(WITH_ORACLE)
  add_subdirectory(oracle)
endif()

if(WITH_POSTGRESQL)
  add_subdirectory(postgresql)
endif()

if(WITH_SQLITE3)
  add_subdirectory(sqlite3)
endif()