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

soci_backend(PostgreSQL
  DEPENDS PostgreSQL
  DESCRIPTION "SOCI backend for PostgreSQL"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Mateusz Loskot")
