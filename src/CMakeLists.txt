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
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(SociUtilities)

#################################################################################
# Find SOCI dependencies
#################################################################################

include(SociDependencies)

#################################################################################
# Installation
#################################################################################

# TODO: configure installation target 

#################################################################################
# Build configured components
#################################################################################

add_subdirectory(core)
