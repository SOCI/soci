################################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009-2010 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
################################################################################

colormsg(_HIBLUE_ "Configuring SOCI core library:")
message(STATUS "")

# Core dependencies
if(Boost_FOUND)
  set_directory_properties(PROPERTIES
    INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})
  add_definitions(-DHAVE_BOOST=1)
endif()

if(Boost_DATE_TIME_FOUND)
  add_definitions(-DHAVE_BOOST_DATE_TIME=1)
endif()

# Core source files
file(GLOB SOCI_CORE_HEADERS *.h)
file(GLOB SOCI_CORE_SOURCES *.cpp)

# Core targets
string(TOLOWER "${PROJECT_NAME}" PROJECTNAMEL)
set(SOCI_CORE_TARGET ${PROJECTNAMEL}_core)

add_library(${SOCI_CORE_TARGET}-static STATIC ${SOCI_CORE_SOURCES})
add_library(${SOCI_CORE_TARGET} SHARED ${SOCI_CORE_SOURCES})

target_link_libraries(${SOCI_CORE_TARGET} "${SOCI_CORE_DEPENDENCIES}")

set_target_properties(${SOCI_CORE_TARGET}-static
  PROPERTIES OUTPUT_NAME ${SOCI_CORE_TARGET})
set_target_properties(${SOCI_CORE_TARGET}
  PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(${SOCI_CORE_TARGET}-static
  PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# TODO: install

# Summary
boost_report_value(SOCI_CORE_TARGET)
boost_report_value(WITH_BOOST)
soci_report_directory_property(COMPILE_DEFINITIONS)

# Export core target name to make it visibla by backends
set(SOCI_CORE_TARGET ${SOCI_CORE_TARGET} PARENT_SCOPE)
