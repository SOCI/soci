# - Find PostgreSQL
# Find the PostgreSQL includes and client library
# This module defines
#  POSTGRESQL_INCLUDE_DIR, where to find POSTGRESQL.h
#  POSTGRESQL_LIBRARIES, the libraries needed to use POSTGRESQL.
#  POSTGRESQL_FOUND, If false, do not try to use PostgreSQL.
#
# Copyright (c) 2010, Mateusz Loskot, <mateusz@loskot.net>
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h
  /usr/include/server
  /usr/include/pgsql/server
  /usr/local/include/pgsql/server
  /usr/include/postgresql
  /usr/include/postgresql/server
  /usr/include/postgresql/*/server
  $ENV{ProgramFiles}/PostgreSQL/*/include
  $ENV{SystemDrive}/PostgreSQL/*/include)

find_library(POSTGRESQL_LIBRARIES NAMES pq libpq
  PATHS
  /usr/lib
  /usr/local/lib
  /usr/lib/postgresql
  /usr/lib64
  /usr/local/lib64
  /usr/lib64/postgresql
  $ENV{ProgramFiles}/PostgreSQL/*/lib/ms
  $ENV{SystemDrive}/PostgreSQL/*/lib/ms)

if(POSTGRESQL_INCLUDE_DIR AND POSTGRESQL_LIBRARIES)
  set(POSTGRESQL_FOUND TRUE)
else()
  set(POSTGRESQL_FOUND FALSE)
endif()

# Handle the QUIETLY and REQUIRED arguments and set POSTGRESQL_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PostgreSQL
  DEFAULT_MSG
  POSTGRESQL_INCLUDE_DIR
  POSTGRESQL_LIBRARIES)

mark_as_advanced(POSTGRESQL_INCLUDE_DIR POSTGRESQL_LIBRARIES)
