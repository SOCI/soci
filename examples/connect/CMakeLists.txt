#
# Copyright (C) 2021 Vadim Zeitlin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

# This is a very simple example of using SOCI in a CMake-based project.

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(SOCIExampleConnect)

find_package(SOCI REQUIRED)

add_executable(connect connect.cpp)

# Note that depending on the SOCI library automatically pulls in the required
# SOCI compilation options too, i.e. there is no need to explicitly use
# target_include_directories().
#
# Linking with just soci_core is enough when using shared libraries, as the
# required backends will be loaded dynamically during run-time, but when using
# static libraries you would need to link with all the soci_<backend> libraries
# needed too.
target_link_libraries(connect PRIVATE SOCI::soci_core)
