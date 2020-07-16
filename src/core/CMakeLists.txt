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

if(WIN32)
    set_target_properties(soci_core PROPERTIES DEFINE_SYMBOL SOCI_DLL)
endif()

target_sources(soci_core 
  PRIVATE
    backend-loader.cpp 
    blob.cpp
    common.cpp
    connection-parameters.cpp
    connection-pool.cpp
    error.cpp
    into-type.cpp
    logger.cpp
    once-temp-type.cpp
    prepare-temp-type.cpp
    procedure.cpp
    ref-counted-prepare-info.cpp
    ref-counted-statement.cpp
    row.cpp
    rowid.cpp
    session.cpp
    soci-simple.cpp
    statement.cpp
    transaction.cpp
    use-type.cpp
    values.cpp
)
target_compile_definitions(soci_core 
  PRIVATE 
    SOCI_LIB_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}soci_"
    SOCI_LIB_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}"
    SOCI_DEBUG_POSTFIX="${CMAKE_DEBUG_POSTFIX}"
)