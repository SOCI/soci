# - Try to find MariaDB / MySQL library
# Find the MySQL includes and client library
# This module defines
#  MySQL_FOUND
#  An interface target MySQL::MySQL to be used in a target_link_libraries call

if (DEFINED MYSQL_INCLUDE_DIR)
  message(DEPRECATION "MYSQL_INCLUDE_DIR has been deprecated. Use MySQL_INCLUDE_DIRS instead")
  set(MySQL_INCLUDE_DIRS "${MYSQL_INCLUDE_DIR}")
endif()
if (DEFINED MYSQL_LIBRARIES)
  message(DEPRECATION "MYSQL_LIBRARIES has been deprecated. Use MySQL_LIBRARIES instead")
  set(MySQL_LIBRARIES "${MYSQL_LIBRARIES}")
endif()


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
    add_library(MySQL::MySQL ALIAS ${LIBRARY_TARGET})

    get_target_property(INCLUDE_DIRS ${LIBRARY_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    if (NOT INCLUDE_DIRS)
      message(FATAL_ERROR "Expected include paths to be set")
    endif()
    foreach(current IN LISTS INCLUDE_DIRS)
      # In SOCI we include the MySQL headers as mysql.h (no prefix) as this is how most MySQL
      # packages seem to be set up and also this seems to be the way MySQL itself advocates.
      # However, the unofficial vcpkg package configures the include path in such a way that
      # we would have to explicitly include mysql/mysql.h (with prefix). Since this is
      # incompatible with the way we include MySQL, we have to modify the include path such
      # that we can omit the prefix as well. This is achieved by appending the mysql directory
      # to the configured include path.
      # In case vcpkg changes this at any point and we end up adding a non-existent directory
      # to the include path, no harm is done (we still keep the original vcpkg paths as well).
      target_include_directories(${LIBRARY_TARGET} SYSTEM INTERFACE "${current}/mysql")
    endforeach()
  endif()
endif()

if (NOT TARGET MySQL::MySQL)
  find_package(PkgConfig QUIET)

  if (PKG_CONFIG_FOUND)
    # Try via PkgConfig
    pkg_search_module(MySQL IMPORTED_TARGET QUIET mysqlclient libmariadb)

    if (TARGET PkgConfig::MySQL)
      add_library(MySQL::MySQL ALIAS PkgConfig::MySQL)
    endif()
  endif()
endif()

if (NOT TARGET MySQL::MySQL)
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
    if (NOT MySQL_INCLUDE_DIRS)
      execute_process(COMMAND ${CONFIG_EXE} --include OUTPUT_VARIABLE MySQL_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)

      # Convert include options of the form -I/dir into just the directories.
      string(REGEX REPLACE "(^| )(-I|-isystem ?)" " " MySQL_INCLUDE_DIRS "${MySQL_INCLUDE_DIRS}")

      # And convert the space-separated string into list.
      separate_arguments(MySQL_INCLUDE_DIRS NATIVE_COMMAND "${MySQL_INCLUDE_DIRS}")
    endif()
    if (NOT MySQL_LIBRARIES)
      execute_process(COMMAND ${CONFIG_EXE} --libs OUTPUT_VARIABLE MySQL_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)

      # Note that we can't remove the -l and/or -L options here, as we can
      # have both of them intermixed, but, luckily, target_link_libraries()
      # accepts both of them directly.
      separate_arguments(MySQL_LIBRARIES NATIVE_COMMAND "${MySQL_LIBRARIES}")
    endif()
    if (NOT MySQL_VERSION)
      execute_process(COMMAND ${CONFIG_EXE} --version OUTPUT_VARIABLE MySQL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
  endif()

  if (NOT MySQL_LIBRARIES)
    message(WARNING "Falling back to manual MySQL search -> this might miss dependencies")
  endif()

  set(MySQL_COMPILE_DEFINITIONS "")

  include(CheckCXXSourceCompiles)

  if(WIN32)
    find_path(MySQL_INCLUDE_DIRS mysql.h
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
    find_path(MySQL_INCLUDE_DIRS mysql.h
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
    set(VERSION_FILE "${MySQL_INCLUDE_DIRS}/mysql_version.h")

    if (EXISTS "${VERSION_FILE}")
      # Parse out MySQL version
      file(READ "${VERSION_FILE}" VERSION_CONTENT)
      string(REGEX MATCH "#define[ \t]+LIBMYSQL_VERSION[ \t]+\"([^\"]+)\"" VERSION_CONTENT "${VERSION_CONTENT}")
      set(MySQL_VERSION "${CMAKE_MATCH_1}")
    endif()
  endif()

  if(WIN32)
     if (CMAKE_BUILD_TYPE)
       string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
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

if (TARGET MySQL::MySQL)
  set(LIB_TARGET MySQL::MySQL)
  set(UNDERLYING_TARGET MySQL::MySQL)

  while (UNDERLYING_TARGET)
    set(LIB_TARGET "${UNDERLYING_TARGET}")
    get_target_property(UNDERLYING_TARGET ${LIB_TARGET} ALIASED_TARGET)
  endwhile()

  if (NOT MySQL_VERSION)
    get_target_property(MySQL_VERSION ${LIB_TARGET} VERSION)
  endif()

  if (NOT MySQL_INCLUDE_DIRS)
    get_target_property(MySQL_INCLUDE_DIRS ${LIB_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  endif()
  if (NOT MySQL_INCLUDE_DIRS)
    get_target_property(MySQL_INCLUDE_DIRS ${LIB_TARGET} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  endif()
  if (NOT MySQL_INCLUDE_DIRS)
    get_target_property(MySQL_INCLUDE_DIRS ${LIB_TARGET} INCLUDE_DIRECTORIES)
  endif()

  if (NOT MySQL_LIBRARIES)
    get_target_property(MySQL_LIBRARIES ${LIB_TARGET} INTERFACE_LINK_LIBRARIES)
  endif()
  if (NOT MySQL_LIBRARIES)
    get_target_property(MySQL_LIBRARIES ${LIB_TARGET} LINK_LIBRARIES)
  endif()

  if (NOT MySQL_VERSION)
    # To prevent printing a weird xyz-NOTFOUND as the version number
    unset(MySQL_VERSION)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
  REQUIRED_VARS MySQL_LIBRARIES MySQL_INCLUDE_DIRS
  VERSION_VAR MySQL_VERSION
)

if (MySQL_FOUND AND NOT TARGET MySQL::MySQL)
  add_library(MySQL INTERFACE)
  target_link_libraries(MySQL INTERFACE ${MySQL_LIBRARIES})
  target_include_directories(MySQL SYSTEM INTERFACE ${MySQL_INCLUDE_DIRS})
  target_compile_options(MySQL INTERFACE ${MySQL_CFLAGS})
  target_link_options(MySQL INTERFACE ${MySQL_LDFLAGS})
  target_compile_definitions(MySQL INTERFACE ${MySQL_COMPILE_DEFINITIONS})
  add_library(MySQL::MySQL ALIAS MySQL)
endif()
