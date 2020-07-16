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
configure_file(soci_version.cpp.in soci_version.cpp @ONLY)
add_library(soci_version STATIC ${CMAKE_CURRENT_BINARY_DIR}/soci_version.cpp)
add_library(SOCI::soci_version ALIAS soci_version)
target_include_directories(soci_version PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

configure_file(soci_backend_config.cpp.in soci_backend_config.cpp @ONLY)
add_library(soci_backend_config STATIC ${CMAKE_CURRENT_BINARY_DIR}/soci_backend_config.cpp)
add_library(SOCI::soci_backend_config ALIAS soci_backend_config)
target_include_directories(soci_backend_config PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/private>)

add_subdirectory(core)
add_subdirectory(backends)
