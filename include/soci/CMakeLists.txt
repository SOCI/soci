# This CMakeLists.txt is a part of SOCI library.
#
# Copyright (C) 2009-2018 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
get_cmake_property(_all_variables CACHE_VARIABLES)

set(CONFIGURED_VARIABLES)
foreach(_v ${_all_variables})
  if (_v MATCHES "^SOCI_HAVE.*")
    get_property(CACHE_HELPSTRING CACHE ${_v} PROPERTY HELPSTRING)
    set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}\n// ${CACHE_HELPSTRING}\n")
    if (${${v}})
      set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}#define ${v}\n")
    else()
      set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}/* #undef ${v} */\n")
    endif()
  endif()
endforeach()

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/soci-config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/soci-config.h")

unset(_all_variables)
unset(CONFIGURED_VARIABLES)

install(FILES
  "${CMAKE_BINARY_DIR}/include/soci/soci-config.h"
  DESTINATION include)
