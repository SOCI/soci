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

option(SOCI_POSTGRESQL_NO_LO64
  "Do not use lo_xxx64() functions for compatibility with PostgreSQL < 9.3"
  OFF)

if (POSTGRESQL_VERSION VERSION_LESS "9.3.0")
  set(SOCI_POSTGRESQL_NO_LO64 ON CACHE BOOL "Avoid using lo_xxx64() functions" FORCE)
endif()

if(SOCI_POSTGRESQL_NO_LO64)
  add_definitions(-DSOCI_POSTGRESQL_NO_LO64=1)
endif()

soci_backend(PostgreSQL
  DEPENDS PostgreSQL
  DESCRIPTION "SOCI backend for PostgreSQL"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Mateusz Loskot")

boost_report_value(SOCI_POSTGRESQL_NO_LO64)
