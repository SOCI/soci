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
add_library(soci_oracle)
add_library(SOCI::soci_oracle ALIAS soci_oracle)
target_link_libraries(soci_oracle PRIVATE SOCI::soci_core SOCI::soci_backend_config ${ORACLE_LIBRARIES})
target_include_directories(soci_oracle PRIVATE ${ORACLE_INCLUDE_DIR})
target_include_directories(soci_oracle PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_oracle PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(WIN32)
    set_target_properties(soci_core PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_oracle
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
