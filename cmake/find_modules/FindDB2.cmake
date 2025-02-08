###############################################################################
# CMake module to search for DB2 client library
#
# On success, the macro sets the following variables:
# DB2_FOUND = if the library found
# DB2_LIBRARIES = full path to the library
# DB2_INCLUDE_DIRS = where to find the library headers
#
# Copyright (c) 2013 Denis Chapligin
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

if(UNIX)
  set(DB2_INSTALL_PATHS
    /opt/ibm/db2/V11.5
    /opt/ibm/db2/V10.1
    /opt/ibm/db2/V9.7
    /opt/ibm/db2/V9.5
    /opt/ibm/db2/V9.1
    /opt/ibm/clidriver
    /opt/clidriver)

  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(DB2_LIBDIRS "lib32" "lib")
  else()
    set(DB2_LIBDIRS "lib64" "lib")
  endif()

  set(DB2_FIND_INCLUDE_PATHS)
  set(DB2_FIND_LIB_PATHS)
  foreach(db2_install_path ${DB2_INSTALL_PATHS})
    if (IS_DIRECTORY ${db2_install_path}/include)
      set(DB2_FIND_INCLUDE_PATHS
        ${DB2_FIND_INCLUDE_PATHS}
        ${db2_install_path}/include)
    endif()
    foreach(db2_libdir ${DB2_LIBDIRS})
      if (IS_DIRECTORY ${db2_install_path}/${db2_libdir})
        set(DB2_FIND_LIB_PATHS
          ${DB2_FIND_LIB_PATHS}
          ${db2_install_path}/${db2_libdir})
      endif()
    endforeach(db2_libdir)
  endforeach(db2_install_path)
elseif(WIN32)
  if (CMAKE_CL_64) # 64-bit build, DB2 64-bit installed
    set(DB2_FIND_INCLUDE_PATHS $ENV{ProgramW6432}/IBM/SQLLIB/include)
    set(DB2_FIND_LIB_PATHS     $ENV{ProgramW6432}/IBM/SQLLIB/lib)
  else() # 32-bit build, DB2 64-bit or DB2 32-bit installed

    if(EXISTS "$ENV{ProgramW6432}/IBM/SQLLIB/lib")
      # On 64-bit Windows with DB2 64-bit installed:
      # LIB environment points to {DB2}/IBM/SQLLIB/lib with64-bit db2api.lib,
      # this flag prevents checking paths in LIB, so Win32 version can be detected
      set(DB2_FIND_LIB_NO_LIB NO_SYSTEM_ENVIRONMENT_PATH)

    endif()

    set(DB2_FIND_INCLUDE_PATHS
      $ENV{ProgramW6432}/IBM/SQLLIB/include
      $ENV{ProgramFiles}/IBM/SQLLIB/include)
    set(DB2_FIND_LIB_PATHS
      $ENV{ProgramFiles}/IBM/SQLLIB/lib
      $ENV{ProgramFiles}/IBM/SQLLIB/lib/win32
      $ENV{ProgramW6432}/IBM/SQLLIB/lib/win32)
  endif()
endif()

find_path(DB2_INCLUDE_DIRS sqlcli1.h
  $ENV{DB2_INCLUDE_DIR}
  $ENV{DB2_INCLUDE_DIRS}
  $ENV{DB2_DIR}/include
  $ENV{DB2_HOME}
  $ENV{IBM_DB_INCLUDE}
  ${DB2_FIND_INCLUDE_PATHS})

find_library(DB2_LIBRARIES
  NAMES db2 db2api
  PATHS
  $ENV{DB2LIB}
  $ENV{IBM_DB_LIB}
  ${DB2_FIND_LIB_PATHS}
  ${DB2_FIND_LIB_NO_LIB})

if(DB2_LIBRARIES)
  get_filename_component(DB2_LIBRARY_DIR ${DB2_LIBRARIES} PATH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DB2
  REQUIRED_VARS DB2_INCLUDE_DIRS DB2_LIBRARIES
)

add_library(DB2 INTERFACE)
target_link_libraries(DB2 INTERFACE ${DB2_LIBRARIES})
target_include_directories(DB2 SYSTEM INTERFACE ${DB2_INCLUDE_DIRS})
add_library(DB2::DB2 ALIAS DB2)
