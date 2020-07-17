###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2013 Viacheslav Naydenov
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
add_library(soci_firebird)
add_library(SOCI::soci_firebird ALIAS soci_firebird)
target_link_libraries(soci_firebird PUBLIC ${FIREBIRD_LIBRARIES} PRIVATE SOCI::soci_core SOCI::soci_backend_config)
target_include_directories(soci_firebird PUBLIC ${FIREBIRD_INCLUDE_DIR})
target_include_directories(soci_firebird PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_firebird PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(soci_firebird PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_firebird
  PRIVATE
    blob.cpp
    common.cpp 
    error-firebird.cpp
    factory.cpp
    session.cpp
    standard-into-type.cpp
    standard-use-type.cpp
    statement.cpp
    vector-into-type.cpp
    vector-use-type.cpp
)
