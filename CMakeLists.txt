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

# BUILD_SHARED_LIBS controlls whether the libraries will be static or shared
configure_file(src/soci_version.cpp.in soci_version.cpp @ONLY)
add_library(soci_version ${CMAKE_CURRENT_BINARY_DIR}/soci_version.cpp)
add_library(SOCI::soci_version ALIAS soci_version)
target_include_directories(soci_version PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

configure_file(src/backends/soci_backend_config.cpp.in soci_backend_config.cpp @ONLY)
add_library(soci_backend_config ${CMAKE_CURRENT_BINARY_DIR}/soci_backend_config.cpp)
add_library(SOCI::soci_backend_config ALIAS soci_backend_config)
target_include_directories(soci_backend_config PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)

add_library(soci_core)
add_library(SOCI::soci_core ALIAS soci_core)
target_link_libraries(soci_core PRIVATE SOCI::soci_backend_config Threads::Threads ${CMAKE_DL_LIBS})
target_include_directories(soci_core PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_core PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(SOCI_WITH_CXX11)
  target_compile_definitions(soci_core PUBLIC SOCI_HAVE_CXX11)
endif()

if(SOCI_WITH_BOOST)
  target_link_libraries(soci_core PRIVATE Boost::boost Boost::date_time)
  target_compile_definitions(soci_core PRIVATE SOCI_HAVE_BOOST)
endif()


if(SOCI_WITH_DB2)
  add_library(soci_db2)
  add_library(SOCI::soci_db2 ALIAS soci_db2)
  target_link_libraries(soci_db2 PRIVATE SOCI::soci_core SOCI::soci_backend_config ${DB2_LIBRARIES})
  target_include_directories(soci_db2 PRIVATE ${DB2_INCLUDE_DIR})
  target_include_directories(soci_db2 PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_db2 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_EMPTY)
  add_library(soci_empty)
  add_library(SOCI::soci_empty ALIAS soci_empty)
  target_link_libraries(soci_empty PRIVATE SOCI::soci_core SOCI::soci_backend_config)
  target_include_directories(soci_empty PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_empty PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_FIREBIRD)
  add_library(soci_firebird)
  add_library(SOCI::soci_firebird ALIAS soci_firebird)
  target_link_libraries(soci_firebird PRIVATE SOCI::soci_core SOCI::soci_backend_config ${FIREBIRD_LIBRARIES})
  target_include_directories(soci_firebird PRIVATE ${FIREBIRD_INCLUDE_DIR})
  target_include_directories(soci_firebird PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_firebird PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_MYSQL)
  add_library(soci_mysql)
  add_library(SOCI::soci_mysql ALIAS soci_mysql)
  target_link_libraries(soci_mysql PRIVATE SOCI::soci_core SOCI::soci_backend_config ${MYSQL_LIBRARIES})
  target_include_directories(soci_mysql PRIVATE ${MYSQL_INCLUDE_DIR})
  target_include_directories(soci_mysql PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_mysql PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_ODBC)
  add_library(soci_odbc)
  add_library(SOCI::soci_odbc ALIAS soci_odbc)
  target_link_libraries(soci_odbc PRIVATE SOCI::soci_core SOCI::soci_backend_config ODBC::ODBC)
  target_include_directories(soci_odbc PRIVATE ${ODBC_INCLUDE_DIRS})
  target_include_directories(soci_odbc PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_odbc PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_ORACLE)
  add_library(soci_oracle)
  add_library(SOCI::soci_oracle ALIAS soci_oracle)
  target_link_libraries(soci_oracle PRIVATE SOCI::soci_core SOCI::soci_backend_config ${ORACLE_LIBRARIES})
  target_include_directories(soci_oracle PRIVATE ${ORACLE_INCLUDE_DIR})
  target_include_directories(soci_oracle PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_oracle PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_POSTGRESQL)
  add_library(soci_postgresql)
  add_library(SOCI::soci_postgresql ALIAS soci_postgresql)
  target_link_libraries(soci_postgresql PRIVATE SOCI::soci_core SOCI::soci_backend_config PostgreSQL::PostgreSQL)
  target_include_directories(soci_postgresql PRIVATE ${PostgreSQL_INCLUDE_DIRS})
  target_include_directories(soci_postgresql PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_postgresql PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

if(SOCI_WITH_SQLITE3)
  add_library(soci_sqlite3)
  add_library(SOCI::soci_sqlite3 ALIAS soci_sqlite3)
  target_link_libraries(soci_sqlite3 PUBLIC SQLite::SQLite3 PRIVATE SOCI::soci_core SOCI::soci_backend_config)
  target_include_directories(soci_sqlite3 PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
  target_include_directories(soci_sqlite3 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()

add_subdirectory(src)

###############################################################################
# Installation
###############################################################################
include(InstallSoci)

INSTALL (DIRECTORY ${PROJECT_SOURCE_DIR}/include/soci DESTINATION include)

install_component(NAME Soci TARGETS soci_core soci_version soci_backend_config)

if(SOCI_WITH_DB2)
  install_component(NAME SociDb2 TARGETS soci_db2)
endif() 

if(SOCI_WITH_EMPTY)
  install_component(NAME SociEmpty TARGETS soci_empty)
endif() 

if(SOCI_WITH_FIREBIRD)
  install_component(NAME SociFirebird TARGETS soci_firebird)
endif() 

if(SOCI_WITH_MYSQL)
  install_component(NAME SociMySQL TARGETS soci_mysql)
endif() 

if(SOCI_WITH_ODBC)
  install_component(NAME SociOdbc TARGETS soci_odbc)
endif() 

if(SOCI_WITH_ORACLE)
  install_component(NAME SociOracle TARGETS soci_oracle)
endif() 

if(SOCI_WITH_POSTGRESQL)
  install_component(NAME SociPostgresql TARGETS soci_postgresql)
endif() 

if(SOCI_WITH_SQLITE3)
  install_component(NAME SociSqlite3 TARGETS soci_sqlite3)
endif() 

###############################################################################
# Tests
###############################################################################
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
