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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

###############################################################################
# Project wide setup
##############################################################################
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put the libaries and binaries at the top of the build tree for easier use
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###############################################################################
# Dependencies
###############################################################################

# Soci core
find_package(Threads REQUIRED)

find_package(Boost COMPONENTS date_time)

# SQLite3 soci backend
find_package(SQLite3 REQUIRED)

###############################################################################
# Main build targets
###############################################################################

# BUILD_SHARED_LIBS controlls whether the libraries will be static or shared
configure_file(src/soci_version.cpp.in soci_version.cpp @ONLY)
add_library(soci_version ${CMAKE_CURRENT_BINARY_DIR}/soci_version.cpp)
target_include_directories(soci_version PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

configure_file(src/backends/soci_backend_config.cpp.in soci_backend_config.cpp @ONLY)
add_library(soci_backend_config ${CMAKE_CURRENT_BINARY_DIR}/soci_backend_config.cpp)
target_include_directories(soci_backend_config PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)

add_library(soci_core)
target_link_libraries(soci_core PRIVATE Threads::Threads Boost::boost Boost::date_time ${CMAKE_DL_LIBS})
target_include_directories(soci_core PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_core PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

add_library(soci_sqlite3)
target_link_libraries(soci_sqlite3 PUBLIC SQLite::SQLite3 PRIVATE soci_core soci_backend_config)
target_include_directories(soci_sqlite3 PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_core PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

add_subdirectory(src)

###############################################################################
# Installation
###############################################################################
include(InstallSoci)

install_component(NAME Soci TARGETS soci_core soci_version)
install_component(NAME SociSqlite3 TARGETS soci_sqlite3 soci_backend_config)

###############################################################################
# Tests
###############################################################################
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
