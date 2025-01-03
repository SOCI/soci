#
# Copyright (C) 2021 Vadim Zeitlin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

# This is a very simple example of using SOCI in a CMake-based project.

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(SOCIExampleConnect)

find_package(SOCI REQUIRED)

add_executable(connect connect.cpp)

# Note that linking to SOCI::soci is enough to inherit
# all of SOCI's dependencies, include directories, etc.
target_link_libraries(connect PRIVATE SOCI::soci)
