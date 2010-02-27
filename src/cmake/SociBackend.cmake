################################################################################
# SociBackend.cmake - part of CMake configuration of SOCI library
################################################################################
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################
# Macros in this module:
#   
#   soci_backend - defines a database backend for SOCI library
#
################################################################################

# Defines a database backend for SOCI library
#
# soci_backend(backendname
#              HEADERS header1 header2
#              HDEPENDS dependency1 dependency2
#              HDESCRIPTION description
#              HAUTHORS author1 author2
#              HMAINTAINERS maintainer1 maintainer2)
#
macro(soci_backend NAME)
  parse_arguments(THIS_BACKEND
    "HEADERS;DEPENDS;DESCRIPTION;AUTHORS;MAINTAINERS;"
    ""
    ${ARGN})

  message("")
  colormsg(HIGREEN "${NAME} - ${THIS_BACKEND_DESCRIPTION}")

  # Backend name variants utils
  string(TOLOWER "${PROJECT_NAME}" PROJECTNAMEL)
  string(TOLOWER "${NAME}" NAMEL)
  string(TOUPPER "${NAME}" NAMEU)

  # Backend option available to user
  set(THIS_BACKEND_OPTION SOCI_${NAMEU})
  option(${THIS_BACKEND_OPTION}
    "Configure and build ${PROJECT_NAME} backend for ${NAME}" ON)

  # Determine required dependencies
  set(THIS_BACKEND_DEPENDS_INCLUDE_DIRS)
  set(THIS_BACKEND_DEPENDS_LIBRARIES)
  set(DEPENDS_NOT_FOUND)

  foreach(dep IN LISTS THIS_BACKEND_DEPENDS)

    soci_check_package_found(${dep} DEPEND_FOUND)
    if(NOT DEPEND_FOUND)
      list(APPEND DEPENDS_NOT_FOUND ${dep}) 
    else()
      string(TOUPPER "${dep}" DEPU)
      list(APPEND THIS_BACKEND_DEPENDS_INCLUDE_DIRS ${${DEPU}_INCLUDE_DIR})
      list(APPEND THIS_BACKEND_DEPENDS_LIBRARIES ${${DEPU}_LIBRARIES})
    endif()
  endforeach()

  list(LENGTH DEPENDS_NOT_FOUND NOT_FOUND_COUNT)
  if (NOT_FOUND_COUNT GREATER 0)
    colormsg(_RED_ "WARNING:")
    colormsg(RED "Some required dependencies of ${NAME} backend not found:")
    foreach(dep IN LISTS DEPENDS_NOT_FOUND)
      colormsg(RED "   ${dep}")
    endforeach()
    # TODO: Abord or warn compilation may fail? --mloskot
    colormsg(RED "Skipping")

    set(${THIS_BACKEND_OPTION} OFF)
  else()

    include_directories(${THIS_BACKEND_DEPENDS_INCLUDE_DIRS})

    # Backend  installable headers and sources
    if (NOT THIS_BACKEND_HEADERS)
      file(GLOB THIS_BACKEND_HEADERS *.h)
    endif()
    file(GLOB THIS_BACKEND_SOURCES *.cpp)
    set(THIS_BACKEND_HEADERS_VAR SOCI_${NAMEU}_HEADERS)
    set(${THIS_BACKEND_HEADERS_VAR} ${THIS_BACKEND_HEADERS}) 

    # Backend target
    set(THIS_BACKEND_TARGET ${PROJECTNAMEL}_${NAMEL})
    set(THIS_BACKEND_TARGET_VAR SOCI_${NAMEU}_TARGET)
    set(${THIS_BACKEND_TARGET_VAR} ${THIS_BACKEND_TARGET})

    # TODO: Add static target 
    #add_library(${THIS_BACKEND_TARGET} STATIC ${THIS_BACKEND_SOURCES})
    add_library(${THIS_BACKEND_TARGET} SHARED ${THIS_BACKEND_SOURCES})

    target_link_libraries(${THIS_BACKEND_TARGET}
      ${THIS_BACKEND_DEPENDS_LIBRARIES})

    # TODO: install
  endif()

  boost_report_value(${THIS_BACKEND_OPTION})
  if(${THIS_BACKEND_OPTION})
    boost_report_value(${THIS_BACKEND_TARGET_VAR})
    boost_report_value(${THIS_BACKEND_HEADERS_VAR})
  endif()

  # LOG
  #message("soci_backend:")
  #message("NAME: ${NAME}")
  #message("${THIS_BACKEND_OPTION} = ${SOCI_BACKEND_SQLITE3}")
  #message("DEPENDS: ${THIS_BACKEND_DEPENDS}")
  #message("DESCRIPTION: ${THIS_BACKEND_DESCRIPTION}")
  #message("AUTHORS: ${THIS_BACKEND_AUTHORS}")
  #message("MAINTAINERS: ${THIS_BACKEND_MAINTAINERS}")
  #message("HEADERS: ${THIS_BACKEND_HEADERS}")
  #message("SOURCES: ${THIS_BACKEND_SOURCES}")
  #message("DEPENDS_LIBRARIES: ${THIS_BACKEND_DEPENDS_LIBRARIES}")
  #message("DEPENDS_INCLUDE_DIRS: ${THIS_BACKEND_DEPENDS_INCLUDE_DIRS}")
endmacro()
