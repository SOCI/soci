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
# SOCI CMake modules
#################################################################################

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(SociUtilities)

message(STATUS "")
colormsg(_HIBLUE_ "Configuring SOCI:")

#################################################################################
# SOCI version information
#################################################################################
include(SociVersion)

soci_version(MAJOR 3 MINOR 1 PATCH 0)

#################################################################################
# Build features and variants
#################################################################################

option(SOCI_TESTS "Enable build of collection of SOCI tests" ON)

boost_report_value(SOCI_TESTS)

#################################################################################
# Find SOCI dependencies
#################################################################################

set(SOCI_CORE_DEPENDENCIES)

include(SociDependencies)

#message("${SOCI_CORE_DEPENDENCIES}")

#################################################################################
# Installation
#################################################################################

set(BINDIR "bin" CACHE PATH "The directory to install binaries into.")
set(LIBDIR "lib" CACHE PATH "The directory to install libraries into.")
set(DATADIR "share" CACHE PATH "The directory to install data files into.")
set(INCLUDEDIR "include" CACHE PATH "The directory to install includes into.")

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
