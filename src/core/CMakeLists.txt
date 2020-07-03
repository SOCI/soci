###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
target_sources(soci_core 
  PRIVATE
    backend-loader.cpp 
    blob.cpp
    common.cpp
    connection-parameters.cpp
    connection-pool.cpp
    error.cpp
    into-type.cpp
    logger.cpp
    once-temp-type.cpp
    prepare-temp-type.cpp
    procedure.cpp
    ref-counted-prepare-info.cpp
    ref-counted-statement.cpp
    row.cpp
    rowid.cpp
    session.cpp
    soci-simple.cpp
    statement.cpp
    transaction.cpp
    use-type.cpp
    values.cpp
)
