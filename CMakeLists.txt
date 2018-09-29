# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

################################################################################
# Project settings
project(GEOS VERSION 4.0.0 LANGUAGES C CXX
  HOMEPAGE_URL "http://soci.sourceforge.net"
  DESCRIPTION "SOCI - The C++ Database Access Library")

################################################################################
# C++ standard
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
# Target compile_options: common C++ compilation flags
add_library(compile_options INTERFACE)
target_compile_features(compile_options INTERFACE cxx_std_98)
# TODO: detect developer's build mode (.git/HEAD exists),
#       and add developer_compile_options target with compile diagnostics
#       See Vtk-m project.

################################################################################
include(include/soci/CMakeLists.txt)
include(src/core/CMakeLists.txt)
include(src/backends/empty/CMakeLists.txt)
#include(src/backends/sqlite3/CMakeLists.txt)

################################################################################
# Install and export targets - support 'make install' or equivalent
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config-version.cmake"
  VERSION ${SOCI_VERSION}
  COMPATIBILITY SameMajorVersion)

configure_file(cmake/soci-config.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config.cmake"
  COPYONLY)

install(
  TARGETS
    compile_options
    soci_core
    soci_empty
  EXPORT soci-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)

install(EXPORT soci-targets
  FILE soci-targets.cmake
  DESTINATION lib/cmake/SOCI
  NAMESPACE SOCI::)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config-version.cmake"
  DESTINATION lib/cmake/SOCI)

install(DIRECTORY
  "${CMAKE_CURRENT_LIST_DIR}/include/soci"
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/soci_backends_config.h"
  "${CMAKE_CURRENT_BINARY_DIR}/soci/soci-config.h"
  DESTINATION include)

################################################################################
# Export targets - support target consumption from the build directory
# TODO

################################################################################
# Summary
message(STATUS "${PROJECT_NAME} configuration summary:")
message(STATUS "  Version:                     ${PROJECT_VERSION}")
message(STATUS "  BUILD_SHARED_LIBS:           ${BUILD_SHARED_LIBS}")
message(STATUS "  BUILD_TESTING:               ${BUILD_TESTING}")
