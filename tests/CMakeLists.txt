###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

colormsg(_HIBLUE_ "Configuring SOCI tests:")

# This works around a problem when building in C++11 mode with clang (see #984).
add_definitions(-DCATCH_CONFIG_CPP11_NO_SHUFFLE)

if(MSVC)
  add_compile_options(/bigobj)
endif()

include_directories(
  ${SOCI_SOURCE_DIR}/include/private
  ${CMAKE_CURRENT_SOURCE_DIR})

# header and source list for the shared test library
set(SOCI_TESTS_COMMON_HEADERS test-assert.h test-context.h test-myint.h)
set(SOCI_TESTS_COMMON_SOURCES
  common/test-main.cpp
  common/test-boost.cpp
  common/test-common.cpp
  common/test-connparams.cpp
  common/test-custom.cpp
  common/test-dynamic.cpp
  common/test-lob.cpp
  common/test-manual.cpp
  common/test-rowset.cpp)

# shared test library targets. we need a target for each version of
# shared/static link against SOCI targets for correct symbol import/visibility
# semantics. more so an issue on Windows (inconsistent symbol annotation)

add_library(soci_tests_common STATIC
  ${SOCI_TESTS_COMMON_HEADERS} ${SOCI_TESTS_COMMON_SOURCES})
# declarations are expected to import from a DSO/DLL
target_compile_definitions(soci_tests_common PUBLIC SOCI_DLL)

add_library(soci_tests_common_static STATIC
  ${SOCI_TESTS_COMMON_HEADERS} ${SOCI_TESTS_COMMON_SOURCES})

add_subdirectory(empty)
add_subdirectory(db2)
add_subdirectory(firebird)
add_subdirectory(mysql)
add_subdirectory(odbc)
add_subdirectory(oracle)
add_subdirectory(postgresql)
add_subdirectory(sqlite3)
