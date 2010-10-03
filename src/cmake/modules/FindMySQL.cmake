# TODO: Change to use mysqlconfig
# TODO: Check library version

find_path(MYSQL_INCLUDE_DIR mysql.h
  $ENV{MYSQL_INCLUDE_DIR}
  $ENV{MYSQL_DIR}/include
  /usr/include/mysql
  /usr/local/include/mysql
  /opt/mysql/mysql/include
  /opt/mysql/mysql/include/mysql
  /usr/local/mysql/include
  /usr/local/mysql/include/mysql
  $ENV{ProgramFiles}/MySQL/*/include
  $ENV{SystemDrive}/MySQL/*/include)

if(WIN32)
  # Set lib path suffixes
  # dist = for mysql binary distributions
  # build = for custom built tree
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(libsuffixDist debug)
    set(libsuffixBuild Debug)
  else()
    set(libsuffixDist opt)
    set(libsuffixBuild Release)
    add_definitions(-DDBUG_OFF)
  endif()

  # On Windows, link against dynamic library libmysql, not static mysqlclient
  find_library(MYSQL_LIBRARY NAMES libmysql
    PATHS
    $ENV{MYSQL_DIR}/lib/${libsuffixDist}
    $ENV{MYSQL_DIR}/libmysql
    $ENV{MYSQL_DIR}/libmysql/${libsuffixBuild}
    $ENV{MYSQL_DIR}/client/${libsuffixBuild}
    $ENV{MYSQL_DIR}/libmysql/${libsuffixBuild}
    $ENV{ProgramFiles}/MySQL/*/lib/${libsuffixDist}
    $ENV{SystemDrive}/MySQL/*/lib/${libsuffixDist})

else()

  find_library(MYSQL_LIBRARY NAMES mysqlclient_r
    PATHS
    $ENV{MYSQL_DIR}/libmysql_r/.libs
    $ENV{MYSQL_DIR}/lib
    $ENV{MYSQL_DIR}/lib/mysql
    /usr/lib/mysql
    /usr/local/lib/mysql
    /usr/local/mysql/lib
    /usr/local/mysql/lib/mysql
    /opt/mysql/mysql/lib
    /opt/mysql/mysql/lib/mysql)
endif()

if(MYSQL_LIBRARY)
  get_filename_component(MYSQL_LIBRARY_DIR ${MYSQL_LIBRARY} PATH)
endif()

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY_DIR)
  set(MYSQL_FOUND TRUE)

  include_directories(${MYSQL_INCLUDE_DIR})
  link_directories(${MYSQL_LIBRARY_DIR})

  find_library(MYSQL_ZLIB zlib PATHS ${MYSQL_LIBRARY_DIR})
  find_library(MYSQL_YASSL yassl PATHS ${MYSQL_LIBRARY_DIR})
  find_library(MYSQL_TAOCRYPT taocrypt PATHS ${MYSQL_LIBRARY_DIR})

  if(WIN32)
    set(MYSQL_CLIENT_LIBS mysqlclient)
  else()
    set(MYSQL_CLIENT_LIBS libmysql)
  endif()

  if(MYSQL_ZLIB)
    set(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} zlib)
  endif()

  if(MYSQL_YASSL)
    set(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} yassl)
  endif()

  if(MYSQL_TAOCRYPT)
    set(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} taocrypt)
  endif()

  # Added needed mysqlclient dependencies on Windows
  if(WIN32)
    set(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} ws2_32)
  endif()
endif()

set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})

# Handle the QUIETLY and REQUIRED arguments and set SQLITE3_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
  DEFAULT_MSG
  MYSQL_INCLUDE_DIR
  MYSQL_LIBRARIES)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
