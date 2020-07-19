###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2011 Denis Chapligin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
add_library(soci_db2)
add_library(SOCI::soci_db2 ALIAS soci_db2)
target_link_libraries(soci_db2 PUBLIC ${DB2_LIBRARIES} PRIVATE SOCI::soci_core SOCI::soci_backend_config)
target_include_directories(soci_db2 PUBLIC ${DB2_INCLUDE_DIR})
target_include_directories(soci_db2 PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_db2 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(soci_db2 PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_db2 
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