# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
add_library(soci_empty "")
set_target_properties(soci_empty PROPERTIES EXPORT_NAME empty)

target_link_libraries(soci_empty PUBLIC compile_options)

target_include_directories(soci_empty
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

target_sources(soci_empty
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/blob.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/factory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/row-id.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/session.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/standard-into-type.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/standard-use-type.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/statement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/vector-into-type.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/vector-use-type.cpp")

if(BUILD_SHARED_LIBS)
  set_target_properties(soci_empty PROPERTIES VERSION ${SOCI_VERSION})
  if(NOT WIN32)
    set_target_properties(soci_empty PROPERTIES SOVERSION ${SOCI_VERSION_MAJOR})
  endif()
endif()
