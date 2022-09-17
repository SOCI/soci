################################################################################
# SociDependencies.cmake - part of CMake configuration of SOCI library
#
# Based on BoostExternals.cmake from CMake configuration for Boost
################################################################################
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
# Copyright (C) 2009 Troy Straszheim
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################
# Macros in this module:
#
#   soci_backend - defines a database backend for SOCI library
#
################################################################################

#
# List of SOCI dependncies
#
set(SOCI_CORE_DEPENDENCIES
  Threads
  Boost)

set(SOCI_BACKENDS_DB_DEPENDENCIES
  MySQL
  ODBC
  Oracle
  PostgreSQL
  SQLite3
  Firebird
  DB2)

set(SOCI_ALL_DEPENDENCIES
  ${SOCI_CORE_DEPENDENCIES}
  ${SOCI_BACKENDS_DB_DEPENDENCIES})

#
# Perform checks
#
colormsg(_HIBLUE_ "Looking for SOCI dependencies:")

macro(boost_external_report NAME)

  set(VARNAME ${NAME})
  string(TOUPPER ${NAME} VARNAMEU)

  set(VARNAMES ${ARGV})
  list(REMOVE_AT VARNAMES 0)

  # Test both, given original name and uppercase version too
  if(NOT ${VARNAME}_FOUND AND NOT ${VARNAMEU}_FOUND)
    colormsg(_RED_ "WARNING: ${NAME} libraries not found, some features will be disabled.")
  endif()

  foreach(variable ${VARNAMES})
    if(${VARNAMEU}_FOUND)
      boost_report_value(${VARNAMEU}_${variable})
    elseif(${VARNAME}_FOUND)
      boost_report_value(${VARNAME}_${variable})
    endif()
  endforeach()
endmacro()

#
#  Some externals default to OFF
#
option(WITH_VALGRIND "Run tests under valgrind" OFF)

#
# Detect available dependencies
#
foreach(external ${SOCI_ALL_DEPENDENCIES})
  string(TOUPPER "${external}" EXTERNAL)

  # For historical reasons we use both WITH_xxx and SOCI_xxx options, and
  # setting either of them to e.g. OFF should disable the corresponding
  # dependency.
  option(WITH_${EXTERNAL} "Attempt to find and configure ${external}" ON)
  option(SOCI_${EXTERNAL} "Attempt to build ${external} backend" ON)

  if(NOT WITH_${EXTERNAL})
    set(disabled_var "WITH")
  elseif(NOT SOCI_${EXTERNAL})
    set(disabled_var "SOCI")
  endif()

  if(NOT DEFINED disabled_var)
    colormsg(HICYAN "${external}:")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/${external}.cmake)
  else()
    set(${EXTERNAL}_FOUND FALSE CACHE BOOL "${external} found" FORCE)
    colormsg(HICYAN "${external}:" YELLOW "disabled, since ${disabled_var}_${EXTERNAL}=OFF")
    unset(disabled_var)
  endif()

  if(NOT ${EXTERNAL}_FOUND)
    set(SOCI_${EXTERNAL} OFF)
  endif()
endforeach()
