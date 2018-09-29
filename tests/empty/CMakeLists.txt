# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
add_executable(test_empty test-empty.cpp)
target_link_libraries(test_empty
  PUBLIC
  soci_core
  z)
