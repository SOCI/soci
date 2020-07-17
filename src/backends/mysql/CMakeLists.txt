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
add_library(soci_mysql)
add_library(SOCI::soci_mysql ALIAS soci_mysql)
target_link_libraries(soci_mysql PUBLIC ${MYSQL_LIBRARIES} PRIVATE SOCI::soci_core SOCI::soci_backend_config)
target_include_directories(soci_mysql PUBLIC ${MYSQL_INCLUDE_DIR})
target_include_directories(soci_mysql PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)
target_include_directories(soci_mysql PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(soci_mysql PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_mysql
  PRIVATE
    blob.cpp
    common.cpp 
    factory.cpp
    row-id.cpp
    session.cpp
    standard-into-type.cpp
    standard-use-type.cpp
    statement.cpp
    vector-into-type.cpp
    vector-use-type.cpp
)
