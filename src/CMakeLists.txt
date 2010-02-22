#################################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009 Mateusz Loskot
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
# SOCI version informatoin
#################################################################################

set(SOCI_VERSION 3.1.0)

#################################################################################
# SOCI CMake modules
#################################################################################

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(SociUtilities)

#################################################################################
# Force CMake options
#################################################################################

set(CMAKE_VERBOSE_MAKEFILE TRUE
  CACHE BOOLEAN "Create verbose makefiles if on" FORCE)

#################################################################################
# Find SOCI dependencies
#################################################################################

set(SOCI_CORE_DEPENDENCIES)
set(SOCI_BACKENDS_DEPENDENCIES)

include(SociDependencies)

set(DL_FIND_QUIETLY TRUE)
find_package(DL)
if(DL_FOUND)
  set(SOCI_CORE_DEPENDENCIES  ${SOCI_CORE_DEPENDENCIES} ${DL_LIBRARY})
  include_directories(${DL_INCLUDE_DIR})
endif()


set(SOCI_CORE_DEPENDENCIES ${SOCI_CORE_DEPENDENCIES} ${Boost_DATE_TIME_LIBRARY})

#################################################################################
# Installation
#################################################################################

# TODO: configure installation target 

#################################################################################
# Build configured components
#################################################################################

include_directories(${SOCI_SOURCE_DIR}/core)

add_subdirectory(core)
add_subdirectory(backends)
