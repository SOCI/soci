###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
add_library(soci_sqlite3)
add_library(SOCI::soci_sqlite3 ALIAS soci_sqlite3)
target_link_libraries(soci_sqlite3 PUBLIC SQLite::SQLite3 PRIVATE SOCI::soci_core SOCI::soci_backend_config)
target_include_directories(soci_sqlite3 PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_sqlite3 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_sources(soci_sqlite3 
  PRIVATE
    blob.cpp
    error.cpp
    factory.cpp
    row-id.cpp
    session.cpp
    standard-into-type.cpp
    standard-use-type.cpp
    statement.cpp
    vector-into-type.cpp
    vector-use-type.cpp
)
