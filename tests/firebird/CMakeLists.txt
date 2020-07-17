###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
include(AddSociTest)

set(SOCI_FIREBIRD_TEST_CONNSTR ":memory:" CACHE STRING "Connection string used for the firebird backend regression test")

add_soci_test(NAME firebird ARGUMENTS ${SOCI_FIREBIRD_TEST_CONNSTR})
target_sources(soci_firebird_test PRIVATE test-firebird.cpp)
