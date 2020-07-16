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
add_library(soci_postgresql)
add_library(SOCI::soci_postgresql ALIAS soci_postgresql)
target_link_libraries(soci_postgresql PRIVATE SOCI::soci_core SOCI::soci_backend_config PostgreSQL::PostgreSQL)
target_include_directories(soci_postgresql PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(soci_postgresql PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_postgresql PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(soci_postgresql PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_postgresql
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
