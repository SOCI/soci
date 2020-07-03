###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2013 Viacheslav Naydenov
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

target_sources(soci_firebird
  PRIVATE
    blob.cpp
    common.cpp 
    error-firebird.cpp
    factory.cpp
    session.cpp
    standard-into-type.cpp
    standard-use-type.cpp
    statement.cpp
    vector-into-type.cpp
    vector-use-type.cpp
)
