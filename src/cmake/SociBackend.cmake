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
#   soci_backend - defines project of a database backend for SOCI library
#
#   soci_backend_test - defines test project of a database backend for SOCI library
################################################################################

# Defines project of a database backend for SOCI library
#
# soci_backend(backendname
#              HEADERS header1 header2
#              DEPENDS dependency1 dependency2
#              DESCRIPTION description
#              AUTHORS author1 author2
#              MAINTAINERS maintainer1 maintainer2)
#
macro(soci_backend NAME)
  parse_arguments(THIS_BACKEND
    "HEADERS;DEPENDS;DESCRIPTION;AUTHORS;MAINTAINERS;"
    ""
    ${ARGN})

  message(STATUS "")
  colormsg(HIGREEN "${NAME} - ${THIS_BACKEND_DESCRIPTION}")

  # Backend name variants utils
  string(TOLOWER "${PROJECT_NAME}" PROJECTNAMEL)
  string(TOLOWER "${NAME}" NAMEL)
  string(TOUPPER "${NAME}" NAMEU)

  # Backend option available to user
  set(THIS_BACKEND_OPTION SOCI_${NAMEU})
  option(${THIS_BACKEND_OPTION}
    "Attempt to build ${PROJECT_NAME} backend for ${NAME}" ON)

  # Determine required dependencies
  set(THIS_BACKEND_DEPENDS_INCLUDE_DIRS)
  set(THIS_BACKEND_DEPENDS_LIBRARIES)
  set(THIS_BACKEND_DEPENDS_DEFS)
  set(DEPENDS_NOT_FOUND)

  # CMake 2.8+ syntax only:
  #foreach(dep IN LISTS THIS_BACKEND_DEPENDS)
  foreach(dep ${THIS_BACKEND_DEPENDS})

    soci_check_package_found(${dep} DEPEND_FOUND)
    if(NOT DEPEND_FOUND)
      list(APPEND DEPENDS_NOT_FOUND ${dep}) 
    else()
      string(TOUPPER "${dep}" DEPU)
      list(APPEND THIS_BACKEND_DEPENDS_INCLUDE_DIRS ${${DEPU}_INCLUDE_DIR})
      list(APPEND THIS_BACKEND_DEPENDS_INCLUDE_DIRS ${${DEPU}_INCLUDE_DIRS})
      list(APPEND THIS_BACKEND_DEPENDS_LIBRARIES ${${DEPU}_LIBRARIES})
      list(APPEND THIS_BACKEND_DEPENDS_DEFS -DHAVE_${DEPU}=1)
    endif()
  endforeach()

  list(LENGTH DEPENDS_NOT_FOUND NOT_FOUND_COUNT)

  if (NOT_FOUND_COUNT GREATER 0)

    colormsg(_RED_ "WARNING:")
    colormsg(RED "Some required dependencies of ${NAME} backend not found:")

    if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)
      foreach(dep ${DEPENDS_NOT_FOUND})
        colormsg(RED "   ${dep}")
      endforeach()
    else()
      foreach(dep IN LISTS DEPENDS_NOT_FOUND)
        colormsg(RED "   ${dep}")
      endforeach()
    endif()

    # TODO: Abord or warn compilation may fail? --mloskot
    colormsg(RED "Skipping")

    set(${THIS_BACKEND_OPTION} OFF)

  else(NOT_FOUND_COUNT GREATER 0)

    if(${THIS_BACKEND_OPTION})

    # Backend-specific include directories
    list(APPEND THIS_BACKEND_DEPENDS_INCLUDE_DIRS ${SOCI_SOURCE_DIR}/core)
    set_directory_properties(PROPERTIES
      INCLUDE_DIRECTORIES "${THIS_BACKEND_DEPENDS_INCLUDE_DIRS}")
    #message("${THIS_BACKEND_DEPENDS_INCLUDE_DIRS}")

    # Backend-specific preprocessor definitions
    add_definitions(${THIS_BACKEND_DEPENDS_DEFS})

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
    add_library(${THIS_BACKEND_TARGET}-static STATIC ${THIS_BACKEND_SOURCES})
    add_library(${THIS_BACKEND_TARGET} SHARED ${THIS_BACKEND_SOURCES})

    target_link_libraries(${THIS_BACKEND_TARGET}
      ${SOCI_CORE_TARGET}
      ${THIS_BACKEND_DEPENDS_LIBRARIES})

    set_target_properties(${THIS_BACKEND_TARGET}-static
      PROPERTIES OUTPUT_NAME ${THIS_BACKEND_TARGET})
    set_target_properties(${THIS_BACKEND_TARGET}
      PROPERTIES
      CLEAN_DIRECT_OUTPUT 1
      VERSION ${SOCI_VERSION}
      SOVERSION ${SOCI_SOVERSION})
    set_target_properties(${THIS_BACKEND_TARGET}-static
      PROPERTIES CLEAN_DIRECT_OUTPUT 1)

    INSTALL(FILES ${THIS_BACKEND_HEADERS} DESTINATION ${INCLUDEDIR}/${PROJECTNAMEL}/${NAMEL})
    INSTALL(TARGETS ${THIS_BACKEND_TARGET} ${THIS_BACKEND_TARGET}-static
      LIBRARY DESTINATION ${LIBDIR}
      ARCHIVE DESTINATION ${LIBDIR})

  else()
    colormsg(HIRED "${NAME}" RED "backend disabled, since")
  endif()

  endif(NOT_FOUND_COUNT GREATER 0)

  boost_report_value(${THIS_BACKEND_OPTION})

  if(${THIS_BACKEND_OPTION})
    boost_report_value(${THIS_BACKEND_TARGET_VAR})
    boost_report_value(${THIS_BACKEND_HEADERS_VAR})

    soci_report_directory_property(COMPILE_DEFINITIONS)
    
    #TODO: report actual name of libraries
    #get_target_property(ARCHIVE_OUTPUT_NAME soci_sqlite3 ARCHIVE_OUTPUT_NAME)
    #boost_report_value(ARCHIVE_OUTPUT_NAME)
    #get_target_property(LIBRARY_OUTPUT_NAME ${THIS_BACKEND_TARGET} LIBRARY_OUTPUT_NAME)
    #boost_report_value(LIBRARY_OUTPUT_NAME)
    #get_target_property(RUNTIME_OUTPUT_NAME ${THIS_BACKEND_TARGET} RUNTIME_OUTPUT_NAME)
    #boost_report_value(RUNTIME_OUTPUT_NAME)
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

# Defines test project of a database backend for SOCI library
#
# soci_backend_test(backendname DEPENDS dependency1 dependency2)
#
macro(soci_backend_test NAME)
  parse_arguments(THIS_TEST
    "DEPENDS;"
    ""
    ${ARGN})

  # Backend name variants utils
  string(TOLOWER "${PROJECT_NAME}" PROJECTNAMEL)
  string(TOLOWER "${NAME}" NAMEL)
  string(TOUPPER "${NAME}" NAMEU)
  set(THIS_TEST soci_test_${NAMEL})

  # Backend test options available to user
  set(THIS_TEST_OPTION SOCI_TEST_${NAMEU})
  option(${THIS_TEST_OPTION}
    "Attempt to build test for ${PROJECT_NAME} ${NAME} backend" ON)

  # Check global flags and enable/disable backend test
  if(NOT SOCI_${NAMEU} OR NOT SOCI_TESTS)
    set(${THIS_TEST_OPTION} OFF)
  endif()

  boost_report_value(${THIS_TEST_OPTION})

  if(${THIS_TEST_OPTION})

    set(THIS_TEST_CONNSTR SOCI_TEST_${NAMEU}_CONNSTR)
    set(${THIS_TEST_CONNSTR} ""
        CACHE STRING "Test connection string for ${NAME} test")
  
    boost_report_value(${THIS_TEST_CONNSTR})

    set(THIS_TEST_TARGET soci_test_${NAMEL})
    # TODO: glob all .cpp files in <backend>/test directory
    set(THIS_TEST_SOURCES test-${NAMEL}.cpp)

    include_directories(${SOCI_SOURCE_DIR}/core/test)
    include_directories(${SOCI_SOURCE_DIR}/backends/${NAMEL})

    add_executable(${THIS_TEST_TARGET} ${THIS_TEST_SOURCES})
    add_executable(${THIS_TEST_TARGET}_static ${THIS_TEST_SOURCES})

    target_link_libraries(${THIS_TEST_TARGET}
      ${SOCI_CORE_TARGET}
      ${SOCI_${NAMEU}_TARGET}
      ${${NAMEU}_LIBRARIES})

    target_link_libraries(${THIS_TEST_TARGET}_static
      ${SOCI_CORE_TARGET}-static
      ${SOCI_${NAMEU}_TARGET}-static
      ${${NAMEU}_LIBRARIES}
      ${SOCI_CORE_STATIC_DEPENDENCIES})

    add_test(${THIS_TEST_TARGET}
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${THIS_TEST_TARGET}
      ${${THIS_TEST_CONNSTR}})

    add_test(${THIS_TEST_TARGET}_static
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${THIS_TEST_TARGET}_static
      ${${THIS_TEST_CONNSTR}})

  endif()

  # LOG
  #message("soci_backend_test:")
  #message("NAME: ${NAME}")
  #message("THIS_TEST_SOURCES: ${THIS_TEST_SOURCES}")
  #message("THIS_TEST_TARGET: ${THIS_TEST_TARGET}")
  #message("THIS_TEST_TARGET_static: ${THIS_TEST_TARGET}_static")
  #message("THIS_TEST_CONNSTR: ${${THIS_TEST_CONNSTR}}")

endmacro()