#################################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009-2010 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
#################################################################################
# General settings
#################################################################################
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

project(SOCI)

#################################################################################
# SOCI version information
#################################################################################

set(SOCI_VERSION 3.1.0)

#################################################################################
# SOCI CMake modules
#################################################################################

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(SociUtilities)

#################################################################################
# Force CMake options
#################################################################################

#set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOLEAN "Create verbose makefiles if on" FORCE)

#################################################################################
# Build output location
#################################################################################

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf directories.
# This simplifies manual testing and the use of the build tree rather
# than installed SOCI libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#################################################################################
# Build features and variants
#################################################################################

# TODO

#################################################################################
# Find SOCI dependencies
#################################################################################

set(SOCI_CORE_DEPENDENCIES)

include(SociDependencies)

#message("${SOCI_CORE_DEPENDENCIES}")

#################################################################################
# Installation
#################################################################################

# TODO: configure installation target 

#################################################################################
# Enable CTest support
#################################################################################

#include(CTest)
enable_testing()
# Define "make check" as alias for "make test"
add_custom_target(check COMMAND ctest)

#################################################################################
# Build configured components
#################################################################################

include(SociBackend)

include_directories(${SOCI_SOURCE_DIR}/core)

add_subdirectory(core)
add_subdirectory(backends)
