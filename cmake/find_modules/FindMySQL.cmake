# - Try to find MariaDB / MySQL library
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_LIB_DIR, path to the MYSQL_LIBRARIES
#  MYSQL_FOUND, If false, do not try to use MySQL.

# Copyright (c) 2006-2008, Jarosław Staniek <staniek@kde.org>
# Copyright (c) 2023 Vadim Zeitline <vz-soci@zeitlins.org> (MariaDB support)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)

if(WIN32)
   find_path(MYSQL_INCLUDE_DIR mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_DIR}/include
      $ENV{ProgramFiles}/MySQL/*/include
      $ENV{SystemDrive}/MySQL/*/include
      $ENV{ProgramW6432}/MySQL/*/include
   )
else(WIN32)
   find_path(MYSQL_INCLUDE_DIR mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_DIR}/include
      PATH_SUFFIXES
      mariadb
      mysql
   )
endif(WIN32)

if(WIN32)
   if (${CMAKE_BUILD_TYPE})
    string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
   endif()

   # path suffix for debug/release mode
   # binary_dist: mysql binary distribution
   # build_dist: custom build
   if(CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")
      set(binary_dist debug)
      set(build_dist Debug)
   else(CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")
      ADD_DEFINITIONS(-DDBUG_OFF)
      set(binary_dist opt)
      set(build_dist Release)
   endif(CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")

#   find_library(MYSQL_LIBRARIES NAMES mysqlclient
   set(MYSQL_LIB_PATHS
      $ENV{MYSQL_DIR}/lib/${binary_dist}
      $ENV{MYSQL_DIR}/libmysql/${build_dist}
      $ENV{MYSQL_DIR}/client/${build_dist}
      $ENV{ProgramFiles}/MySQL/*/lib/${binary_dist}
      $ENV{SystemDrive}/MySQL/*/lib/${binary_dist}
      $ENV{MYSQL_DIR}/lib/opt
      $ENV{MYSQL_DIR}/client/release
      $ENV{ProgramFiles}/MySQL/*/lib/opt
      $ENV{SystemDrive}/MySQL/*/lib/opt
      $ENV{ProgramW6432}/MySQL/*/lib
   )
   find_library(MYSQL_LIBRARIES NAMES libmysql
      PATHS
      ${MYSQL_LIB_PATHS}
   )
else(WIN32)
#   find_library(MYSQL_LIBRARIES NAMES mysqlclient
   set(MYSQL_LIB_PATHS
      $ENV{MYSQL_DIR}/lib
      PATH_SUFFIXES
      mariadb
      mysql
   )
   find_library(MYSQL_LIBRARIES NAMES mariadbclient mysqlclient
      PATHS
      ${MYSQL_LIB_PATHS}
   )
endif(WIN32)

if(MYSQL_LIBRARIES)
   get_filename_component(MYSQL_LIB_DIR ${MYSQL_LIBRARIES} PATH)
endif(MYSQL_LIBRARIES)

set( CMAKE_REQUIRED_INCLUDES ${MYSQL_INCLUDE_DIR} )

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND TRUE)
   message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND FALSE)
   message(STATUS "MySQL not found.")
endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
