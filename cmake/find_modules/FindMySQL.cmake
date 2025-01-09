# - Try to find MariaDB / MySQL library
# Find the MySQL includes and client library
# This module defines
#  MySQL_FOUND
#  An interface target MySQL::MySQL to be used in a target_link_libraries call


if (DEFINED VCPKG_TARGET_TRIPLET)
  # In vcpg the MySQL packages are called unofficial-libmysql
  find_package(unofficial-libmysql)

  set(FOUND_VAR "unofficial-libmysql_FOUND")
  set(LIBRARY_TARGET "unofficial::libmysql::libmysql")

  if (NOT ${FOUND_VAR})
    find_package(unofficial-libmariadb QUIET)
    set(FOUND_VAR "unofficial-libmariadb_FOUND")
    set(LIBRARY_TARGET "unofficial::libmariadb::libmariadb")
  endif()

  if (${FOUND_VAR})
    set(MySQL_FOUND TRUE)
    message(STATUS "Found MySQL via vcpkg installation")
    add_library(MySQL::MySQL ALIAS ${LIBRARY_TARGET})
    return()
  endif()
endif()

find_package(PkgConfig QUIET)

if (PKG_CONFIG_FOUND)
  # Try via PkgConfig
  pkg_check_modules(MYSQLCLIENT QUIET mysqlclient)

  if (MYSQLCLIENT_FOUND)
    if (NOT BUILD_SHARED_LIBS AND MYSQLCLIENT_STATIC_FOUND)
      set(PREFIX MYSQLCLIENT_STATIC)
    else()
      set(PREFIX MYSQLCLIENT)
    endif()

    set(MySQL_LIBRARIES ${${PREFIX}_LINK_LIBRARIES})
    set(MySQL_LDFLAGS ${${PREFIX}_LDFLAGS} ${${PREFIX}_LDFLAGS_OTHER})
    set(MySQL_INCLUDE_DIRS ${${PREFIX}_INCLUDE_DIRS} ${${PREFIX}_INCLUDE_DIRS_OTHER})
    set(MySQL_CFLAGS ${${PREFIX}_CFLAGS} ${${PREFIX}_CFLAGS_OTHER})
    set(MySQL_VERSION ${MYSQLCLIENT_VERSION})
  endif()
endif()

if (NOT MySQL_LIBRARIES)
  # Try using config exe
  find_program(CONFIG_EXE
    NAMES
      mysql_config mariadb_config
    PATHS
      $ENV{MYSQL_DIR}
      $ENV{MYSQL_DIRS}
      $ENV{ProgramFiles}/MySQL/
      $ENV{ProgramFiles}/MariaDB/
  )

  if (CONFIG_EXE)
    execute_process(COMMAND ${CONFIG_EXE} --include OUTPUT_VARIABLE MySQL_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CONFIG_EXE} --libs OUTPUT_VARIABLE MySQL_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CONFIG_EXE} --version OUTPUT_VARIABLE MySQL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

if (NOT MySQL_LIBRARIES)
  message(WARNING "Falling back to manual MySQL search -> this might miss dependencies")
  set(MySQL_COMPILE_DEFINITIONS "")

  include(CheckCXXSourceCompiles)

  foreach(TOP_LEVEL_DIR IN ITEMS "mysql/" "")
    if(WIN32)
      find_path(MySQL_INCLUDE_DIRS ${TOP_LEVEL_DIR}mysql.h
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
      find_path(MySQL_INCLUDE_DIRS ${TOP_LEVEL_DIR}mysql.h
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

    if (MySQL_INCLUDE_DIRS)
      if (TOP_LEVEL_DIR STREQUAL "")
        list(APPEND MySQL_COMPILE_DEFINITIONS SOCI_MYSQL_DIRECT_INCLUDE)
        set(VERSION_FILE "${MySQL_INCLUDE_DIRS}/mysql_version.h")
      else()
        set(VERSION_FILE "${MySQL_INCLUDE_DIRS}/mysql/mysql_version.h")
      endif()

      # Parse out MySQL version
      file(READ "${VERSION_FILE}" VERSION_CONTENT)
      string(REGEX MATCH "#define[ \t]+LIBMYSQL_VERSION[ \t]+\"([^\"]+)\"" VERSION_CONTENT "${VERSION_CONTENT}")
      set(MySQL_VERSION "${CMAKE_MATCH_1}")

      break()
    endif()
  endforeach()

  if (MySQL_INCLUDE_DIRS)
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
       list(APPEND MySQL_COMPILE_DEFINITIONS DBUG_OFF)
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
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
  REQUIRED_VARS MySQL_LIBRARIES MySQL_INCLUDE_DIRS
  VERSION_VAR MySQL_VERSION
)

if (MySQL_FOUND)
  add_library(MySQL INTERFACE)
  target_link_libraries(MySQL INTERFACE ${MySQL_LIBRARIES})
  target_include_directories(MySQL SYSTEM INTERFACE ${MySQL_INCLUDE_DIRS})
  target_compile_options(MySQL INTERFACE ${MySQL_CFLAGS})
  target_link_options(MySQL INTERFACE ${MySQL_LDFLAGS})
  target_compile_definitions(MySQL INTERFACE ${MySQL_COMPILE_DEFINITIONS})
  add_library(MySQL::MySQL ALIAS MySQL)
endif()
