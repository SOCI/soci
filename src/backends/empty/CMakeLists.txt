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
add_library(soci_empty)
add_library(SOCI::soci_empty ALIAS soci_empty)
target_link_libraries(soci_empty PRIVATE SOCI::soci_core SOCI::soci_backend_config)
target_include_directories(soci_empty PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_empty PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_sources(soci_empty
  PRIVATE
    blob.cpp
    factory.cpp
    row-id.cpp
    session.cpp
    standard-into-type.cpp
    standard-use-type.cpp
    statement.cpp
    vector-into-type.cpp
    vector-use-type.cpp
)
