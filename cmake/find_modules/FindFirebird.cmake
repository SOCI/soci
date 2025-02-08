##############################################################
# Copyright (c) 2008 Daniel Pfeifer                          #
#                                                            #
# Distributed under the Boost Software License, Version 1.0. #
##############################################################

# This module defines
# Firebird_INCLUDE_DIRS - where to find ibase.h
# Firebird_LIBRARIES - the libraries to link against to use Firebird
# Firebird_FOUND - true if Firebird was found

if (DEFINED FIREBIRD_INCLUDE_DIR)
  message(DEPRECATION "FIREBIRD_INCLUDE_DIR has been deprecated. Use Firebird_INCLUDE_DIRS instead.")
  set(Firebird_INCLUDE_DIRS "${FIREBIRD_INCLUDE_DIR}")
endif()
if (DEFINED FIREBIRD_LIBRARIES)
  message(DEPRECATION "FIREBIRD_LIBRARIES has been deprecated. Use Firebird_LIBRARIES instead.")
  set(Firebird_LIBRARIES "${FIREBIRD_LIBRARIES}")
endif()

find_path(Firebird_INCLUDE_DIRS ibase.h
  /usr/include
  $ENV{ProgramFiles}/Firebird/*/include
)

if (Firebird_SEARCH_EMBEDDED)
  set(Firebird_LIB_NAMES fbembed)
else()
  set(Firebird_LIB_NAMES fbclient fbclient_ms)
endif()

find_library(Firebird_LIBRARIES
  NAMES
    ${Firebird_LIB_NAMES}
  PATHS
    /usr/lib
    $ENV{ProgramFiles}/Firebird/*/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Firebird
  REQUIRED_VARS Firebird_LIBRARIES Firebird_INCLUDE_DIRS
)

add_library(Firebird INTERFACE)
target_link_libraries(Firebird INTERFACE ${Firebird_LIBRARIES})
target_include_directories(Firebird SYSTEM INTERFACE ${Firebird_INCLUDE_DIRS})
add_library(Firebird::Firebird ALIAS Firebird)
