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

set(SOCI_CORE_TARGET soci_core)

file(GLOB SOCI_CORE_HEADERS *.h)
file(GLOB SOCI_CORE_SOURCES *.cpp)

add_library(${SOCI_CORE_TARGET} SHARED ${SOCI_CORE_SOURCES})

target_link_libraries(${SOCI_CORE_TARGET} ${SOCI_CORE_DEPENDENCIES})

set(SOCI_BACKENDS_DEPENDENCIES
  ${SOCI_BACKENDS_DEPENDENCIES}
  ${SOCI_CORE_TARGET}
  PARENT_SCOPE)

# TODO: install
