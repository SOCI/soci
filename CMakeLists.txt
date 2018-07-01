# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

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
# Target soci_cxx_flags: common C++ compilation flags
add_library(soci_cxx_flags INTERFACE)
target_compile_features(soci_cxx_flags INTERFACE cxx_std_98)

################################################################################
include(include/soci/CMakeLists.txt)
include(src/core/CMakeLists.txt)
include(src/backends/empty/CMakeLists.txt)
include(src/backends/sqlite3/CMakeLists.txt)

################################################################################
# Install and export targets - support 'make install' or equivalent
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config-version.cmake"
  VERSION ${SOCI_VERSION}
  COMPATIBILITY SameMajorVersion)
