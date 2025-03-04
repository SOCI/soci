#
# Copyright (C) 2021 Vadim Zeitlin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#

# This is a very simple example of using SOCI in a CMake-based project.

cmake_minimum_required(VERSION 3.10...3.31 FATAL_ERROR)

project(SOCIExampleConnect)

# Note: SOCI versions before 4.1 rely on hand-written FindSOCI.cmake files to be
# present in downstream projects as SOCI didn't use to ship a config file.
# This is changed in 4.1 thanks to which the desired database backends can
# be specified as components. Known component names are
# - Empty
# - DB2
# - Firebird
# - MySQL
# - ODBC
# - Oracle
# - PostgreSQL
# - SQLite3
# note that some components may still be undefined if the corresponding library is
# not included in the SOCI version installed on your system.
find_package(SOCI 4.1 COMPONENTS Empty REQUIRED)

add_executable(connect connect.cpp)

# Note that linking to SOCI::soci is enough to inherit
# all of SOCI's dependencies, include directories, etc.
target_link_libraries(connect PRIVATE SOCI::soci)
