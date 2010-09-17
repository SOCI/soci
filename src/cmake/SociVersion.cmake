################################################################################
# SociVersion.cmake - part of CMake configuration of SOCI library
################################################################################
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################
# Macros in this module:
#   
#   soci_version - defines version information for SOCI library
#
################################################################################

# Defines version information for SOCI library
#
# soci_version(MAJOR major_version MINOR minor_version PATCH patch_level)
#
#    MAJOR.MINOR version is used to set SOVERSION
#
macro(soci_version)
  parse_arguments(THIS_VERSION "MAJOR;MINOR;PATCH;"
    ""
    ${ARGN})

  # Set version components
  set(${PROJECT_NAME}_VERSION_MAJOR ${THIS_VERSION_MAJOR})
  set(${PROJECT_NAME}_VERSION_MINOR ${THIS_VERSION_MINOR})
  set(${PROJECT_NAME}_VERSION_PATCH ${THIS_VERSION_PATCH})

  # Set VERSION string
  set(${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

  # Set SOVERSION based on major
  set(${PROJECT_NAME}_SOVERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}")

  message(STATUS "")

  boost_report_value(SOCI_VERSION)
  boost_report_value(SOCI_SOVERSION)

endmacro()
