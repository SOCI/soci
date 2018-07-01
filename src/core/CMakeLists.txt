# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
add_library(soci_core "")

target_link_libraries(soci_core PUBLIC soci_cxx_flags)

target_include_directories(soci_core
  PRIVATE
    "${CMAKE_SOURCE_DIR}/include/private"
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

target_compile_definitions(soci_core
  PRIVATE
    SOCI_LIB_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}soci_"
    SOCI_LIB_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}"
    SOCI_DEBUG_POSTFIX="${CMAKE_DEBUG_POSTFIX}")

target_sources(soci_core
  PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/backend-loader.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/blob.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/common.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/connection-parameters.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/connection-pool.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/error.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/into-type.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/logger.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/once-temp-type.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/prepare-temp-type.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/procedure.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ref-counted-prepare-info.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ref-counted-statement.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/row.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/rowid.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/soci-simple.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/statement.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/transaction.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/use-type.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/values.cpp")

if(BUILD_SHARED_LIBS)
  set_target_properties(soci_core PROPERTIES VERSION ${SOCI_VERSION})
  if(NOT WIN32)
    set_target_properties(soci_core PROPERTIES SOVERSION ${SOCI_VERSION_MAJOR})
  endif()
endif()

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/soci_backends_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/soci_backends_config.h")
