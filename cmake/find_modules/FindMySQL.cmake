# - Try to find MariaDB / MySQL library
# Find the MySQL includes and client library
# This module defines
#  MySQL_INCLUDE_DIRS
#  MySQL_LIBRARIES, the libraries needed to use MySQL.
#  MySQL_LIB_DIR, path to the MySQL_LIBRARIES
#  MySQL_FOUND, If false, do not try to use MySQL.

# Copyright (c) 2006-2008, Jarosław Staniek <staniek@kde.org>
# Copyright (c) 2023 Vadim Zeitline <vz-soci@zeitlins.org> (MariaDB support)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)

if(WIN32)
  find_path(MySQL_INCLUDE_DIRS mysql/mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_INCLUDE_DIRS}
      $ENV{MYSQL_DIR}/include
      $ENV{MYSQL_DIRS}/include
      $ENV{ProgramFiles}/MySQL/*/include
      $ENV{SystemDrive}/MySQL/*/include
      $ENV{ProgramW6432}/MySQL/*/include
   )
else()
  find_path(MySQL_INCLUDE_DIRS mysql/mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_INCLUDE_DIRS}
      $ENV{MYSQL_DIR}/include
      $ENV{MYSQL_DIRS}/include
      PATH_SUFFIXES
      mariadb
      mysql
   )
endif()

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
   else()
      ADD_DEFINITIONS(-DDBUG_OFF)
      set(binary_dist opt)
      set(build_dist Release)
   endif()

   set(MySQL_LIB_PATHS
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
   find_library(MySQL_LIBRARIES NAMES libmysql
      PATHS
      ${MySQL_LIB_PATHS}
   )
else()
   set(MySQL_LIB_PATHS
      $ENV{MySQL_DIR}/lib
      PATH_SUFFIXES
      mariadb
      mysql
   )
   find_library(MySQL_LIBRARIES NAMES mariadbclient mysqlclient
      PATHS
      ${MySQL_LIB_PATHS}
   )
endif()

if(MySQL_LIBRARIES)
   get_filename_component(MySQL_LIB_DIR ${MySQL_LIBRARIES} PATH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
  REQUIRED_VARS MySQL_LIBRARIES MySQL_INCLUDE_DIRS
)

if (MySQL_FOUND)
  add_library(MySQL INTERFACE)
  target_link_libraries(MySQL INTERFACE ${MySQL_LIBRARIES})
  target_include_directories(MySQL SYSTEM INTERFACE ${MySQL_INCLUDE_DIRS})
  add_library(MySQL::MySQL ALIAS MySQL)
endif()
