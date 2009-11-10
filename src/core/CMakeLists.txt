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

set(SOCI_CORE_LIB_NAME soci_core)

file(GLOB SOCI_CORE_HPP *.h)
file(GLOB SOCI_CORE_CPP *.cpp)

###############################################################################
# core build targets

add_library(${SOCI_CORE_LIB_NAME} SHARED ${SOCI_CORE_CPP})

target_link_libraries(${SOCI_CORE_LIB_NAME}
  ${Boost_date_time_LIBRARY})

###############################################################################
# core installation

# TODO