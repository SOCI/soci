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

include_directories(${SOCI_SOURCE_DIR}/include/private)

add_subdirectory(assert)

message(STATUS "")
