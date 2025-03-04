###############################################################################
#
# CMake module to search for Oracle client library (OCI)
#
# On success, the macro sets the following variables:
# Oracle_FOUND        = if the library found
# Oracle_LIBRARY      = full path to the library
# Oracle_LIBRARIES    = full path to the library
# Oracle_INCLUDE_DIRS = where to find the library headers also defined,
#                       but not for general use are
# Oracle_VERSION     = version of library which was found, e.g. "1.2.5"
#
# Copyright (c) 2009-2013 Mateusz Loskot <mateusz@loskot.net>
#
# Developed with inspiration from Petr Vanek <petr@scribus.info>
# who wrote similar macro for TOra - https://torasql.com/
#
# Module source: https://github.com/mloskot/workshop/tree/master/cmake/
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

if (DEFINED ORACLE_INCLUDE_DIR)
  message(DEPRECATION "ORACLE_INCLUDE_DIR has been deprecated. Use Oracle_INCLUDE_DIRS instead")
  set(Oracle_INCLUDE_DIRS "${ORACLE_INCLUDE_DIR}")
endif()
if (DEFINED ORACLE_LIBRARY)
  message(DEPRECATION "ORACLE_LIBRARY has been deprecated. Use Oracle_LIBRARIES instead")
  set(Oracle_LIBRARIES "${ORACLE_LIBRARY}")
endif()

# First check for CMAKE  variable
if(NOT ORACLE_HOME AND NOT Oracle_HOME)
  if(EXISTS $ENV{ORACLE_HOME})
    set(Oracle_HOME $ENV{ORACLE_HOME})
  endif()
  if(EXISTS $ENV{Oracle_HOME})
    set(Oracle_HOME $ENV{Oracle_HOME})
  endif()
endif()
if (ORACLE_HOME)
  set(Oracle_HOME "${ORACLE_HOME}")
endif()

find_path(Oracle_INCLUDE_DIRS
  NAMES oci.h
  PATHS
  ${Oracle_HOME}/rdbms/public
  ${Oracle_HOME}/include
  ${Oracle_HOME}/sdk/include  # Oracle SDK
  ${Oracle_HOME}/OCI/include # Oracle XE on Windows
  # instant client from rpm
  /usr/include/oracle/*/client${LIB_SUFFIX})

set(Oracle_VERSIONS 21 20 19 18 12 11 10)
set(Oracle_OCI_NAMES clntsh libclntsh oci) # Dirty trick might help on OSX, see issues/89
set(Oracle_OCCI_NAMES libocci occi)
set(Oracle_NNZ_NAMES ociw32)

foreach(loop_var IN LISTS Oracle_VERSIONS)
  set(Oracle_OCCI_NAMES ${Oracle_OCCI_NAMES} oraocci${loop_var})
  set(Oracle_NNZ_NAMES ${Oracle_NNZ_NAMES} nnz${loop_var} libnnz${loop_var})
endforeach(loop_var)

set(Oracle_LIB_DIR
  ${Oracle_HOME}
  ${Oracle_HOME}/lib
  ${Oracle_HOME}/sdk/lib       # Oracle SDK
  ${Oracle_HOME}/sdk/lib/msvc
  ${Oracle_HOME}/OCI/lib/msvc # Oracle XE on Windows
  # Instant client from rpm
  /usr/lib/oracle/*/client${LIB_SUFFIX}/lib)

find_library(Oracle_OCI_LIBRARY
  NAMES ${Oracle_OCI_NAMES} PATHS ${Oracle_LIB_DIR})
find_library(Oracle_OCCI_LIBRARY
  NAMES ${Oracle_OCCI_NAMES} PATHS ${Oracle_LIB_DIR})
find_library(Oracle_NNZ_LIBRARY
  NAMES ${Oracle_NNZ_NAMES} PATHS ${Oracle_LIB_DIR})

if (Oracle_OCI_LIBRARY AND Oracle_OCCI_LIBRARY AND Oracle_NNZ_LIBRARY)
  set(Oracle_LIBRARIES
    ${Oracle_OCI_LIBRARY}
    ${Oracle_OCCI_LIBRARY}
    ${Oracle_NNZ_LIBRARY})
endif()

if(NOT WIN32 AND Oracle_CLNTSH_LIBRARY)
  list(APPEND Oracle_LIBRARIES ${Oracle_CLNTSH_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Oracle
  REQUIRED_VARS Oracle_LIBRARIES Oracle_INCLUDE_DIRS)

if(Oracle_FOUND)
  add_library(Oracle INTERFACE)
  target_link_libraries(Oracle INTERFACE ${Oracle_LIBRARIES})
  target_include_directories(Oracle SYSTEM INTERFACE ${Oracle_INCLUDE_DIRS})
  add_library(Oracle::Oracle ALIAS Oracle)
else()
	message(STATUS "None of the supported Oracle versions (${Oracle_VERSIONS}) could be found, consider updating Oracle_VERSIONS if the version you use is not among them.")
endif()
