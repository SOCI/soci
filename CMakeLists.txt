###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
# Preamble
###############################################################################
cmake_minimum_required(VERSION 3.17)
project(SOCI VERSION 5.0.0 LANGUAGES CXX HOMEPAGE_URL "http://soci.sourceforge.net" DESCRIPTION "SOCI - The C++ Database Access Library") 
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

###############################################################################
# Project wide setup
###############################################################################
option(SOCI_WITH_DB2 "Add Db2 build target and search for dependencies" OFF)
option(SOCI_WITH_EMPTY "Add Empty build target and search for dependencies" ON)
option(SOCI_WITH_FIREBIRD "Add Firebird build target and search for dependencies" OFF)
option(SOCI_WITH_MYSQL "Add Mysql build target and search for dependencies" OFF)
option(SOCI_WITH_ODBC "Add Odbc build target and search for dependencies" OFF)
option(SOCI_WITH_ORACLE "Add Oracle build target and search for dependencies" OFF)
option(SOCI_WITH_POSTGRESQL "Add Firebird build target and search for dependencies" OFF)
option(SOCI_WITH_SQLITE3 "Add Sqlite3 build target and search for dependencies" OFF)
option(SOCI_WITH_BOOST "Add boost support" OFF)
option(SOCI_WITH_CXX11 "Build to the C++11 standard" OFF)

# Check if soci is the top level project or included in another project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  if(SOCI_WITH_CXX11)
    set(CMAKE_CXX_STANDARD 11)
  else() 
    set(CMAKE_CXX_STANDARD 98)
  endif()
  
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Put the libaries and binaries at the top of the build tree for easier use
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  # IDE support for folders
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  include(CTest)
  include(cmake/StaticAnalyzers.cmake)
endif()

###############################################################################
# Dependencies
###############################################################################
add_subdirectory(dependencies)

# Request CATCH to disable all the C++11 features
if (NOT SOCI_WITH_CXX11)
  target_compile_definitions(Catch::Catch INTERFACE CATCH_CONFIG_NO_CPP11)
endif()

# Soci core
find_package(Threads REQUIRED)

# Optional boost support
if(SOCI_WITH_BOOST)
  find_package(Boost COMPONENTS date_time REQUIRED)
endif() 

# DB2 soci backend
if(SOCI_WITH_DB2)
  find_package(DB2 REQUIRED)
endif()

# Firebird soci backend
if(SOCI_WITH_FIREBIRD)
  find_package(Firebird REQUIRED)
endif()

# Firebird soci backend
if(SOCI_WITH_MYSQL)
  find_package(MySQL REQUIRED)
endif()

# ODBC soci backend
if(SOCI_WITH_ODBC)
  find_package(ODBC REQUIRED)
endif()

# Oracle soci backend
if(SOCI_WITH_ORACLE)
  find_package(Oracle REQUIRED)
endif()

# Postgresql soci backend
if(SOCI_WITH_POSTGRESQL)
  find_package(PostgreSQL REQUIRED)
endif()

# SQLite3 soci backend
if(SOCI_WITH_SQLITE3)
  find_package(SQLite3 REQUIRED)
endif()

###############################################################################
# Main build targets
###############################################################################
add_subdirectory(src)

###############################################################################
# Tests
###############################################################################
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

###############################################################################
# Packaging
###############################################################################
add_subdirectory(packaging)
