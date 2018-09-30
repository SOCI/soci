################################################################################
# This soci-config.cmake is a part of SOCI library.
#
# Copyright (C) 2018 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
################################################################################
include(include(CMakeFindDependencyMacro)
find_dependency(SQLite3)

include("${CMAKE_CURRENT_LIST_DIR}/soci-targets.cmake")
