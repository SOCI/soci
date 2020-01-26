###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

include(CMakeDependentOption)

option(SOCI_POSTGRESQL_NOSINGLEROWMODE
  "Do not use single row mode. PostgreSQL <9 portability."
  OFF)

if (POSTGRESQL_VERSION VERSION_LESS "9.0.0")
  set(SOCI_POSTGRESQL_NOSINGLEROWMODE ON CACHE BOOL "Use single row mode for PostgreSQL 9+" FORCE)
endif()

if(SOCI_POSTGRESQL_NOSINGLEROWMODE)
  add_definitions(-DSOCI_POSTGRESQL_NOSINGLEROWMODE=1)
endif()

soci_backend(PostgreSQL
  DEPENDS PostgreSQL
  DESCRIPTION "SOCI backend for PostgreSQL"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Mateusz Loskot")

boost_report_value(SOCI_POSTGRESQL_NOSINLGEROWMODE)
