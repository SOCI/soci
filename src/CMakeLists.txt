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

set(SOCI_CMAKE_VERSION 3.1.0)

#################################################################################
# General features
#################################################################################

# Turn makefile commands in colours
set(CMAKE_COLOR_MAKEFILE ON)

# Allow advanced users to generate Makefiles printing detailed commands
mark_as_advanced(CMAKE_VERBOSE_MAKEFILE)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# TODO: build variants and flags

#################################################################################
# Other general features depending on thirdparty components
#################################################################################

# Boost C++ Libraries support - optional, default=OFF
set(SOCI_WITH_BOOST FALSE CACHE BOOL "Choose if Boost C++ Libraries support should be built")

if(SOCI_WITH_BOOST)
  find_package(Boost 1.33.1 REQUIRED date_time)

  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions(-DHAVE_BOOST)
  endif()

  if(Boost_date_time_FOUND)
    add_definitions(-DHAVE_BOOST_DATE_TIME)
  endif()
endif()

#################################################################################
# Backends-specific features
#################################################################################

#################################################################################
# Installation
#################################################################################

# TODO: configure installation target 

#################################################################################
# Build configured components
#################################################################################

add_subdirectory(core)
