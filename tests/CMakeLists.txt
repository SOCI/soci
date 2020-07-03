###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

# Request CATCH to disable all the C++11 features
if (NOT SOCI_CXX11)
  add_definitions(-DCATCH_CONFIG_NO_CPP11)
endif()


add_executable(soci_sqlite3_test)
target_link_libraries(soci_sqlite3_test PRIVATE soci_core soci_sqlite3)
target_include_directories(soci_sqlite3_test 
  PRIVATE 
    ${SOCI_SOURCE_DIR}/include/private 
    ${CMAKE_CURRENT_LIST_DIR}
)

add_test(NAME soci_sqlite3 COMMAND soci_sqlite3_test)

# add_subdirectory(empty)
# add_subdirectory(db2)
# add_subdirectory(firebird)
# add_subdirectory(mysql)
# add_subdirectory(odbc)
# add_subdirectory(oracle)
# add_subdirectory(postgresql)
add_subdirectory(sqlite3)
