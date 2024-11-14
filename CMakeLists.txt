###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2009-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
# General settings
###############################################################################
cmake_minimum_required(VERSION 2.8...3.20 FATAL_ERROR)

project(SOCI)

if(NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${SOCI_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(SociUtilities)

###############################################################################
# Build features and variants
##############################################################################

option(SOCI_SHARED "Enable build of shared libraries" ON)
option(SOCI_STATIC "Enable build of static libraries" ON)
option(SOCI_TESTS "Enable build of collection of SOCI tests" ON)
option(SOCI_ASAN "Enable address sanitizer on GCC v4.8+/Clang v 3.1+" OFF)
option(SOCI_UBSAN "Enable undefined behaviour sanitizer" OFF)
option(SOCI_LTO "Enable link time optimization" OFF)
option(SOCI_VISIBILITY "Enable hiding private symbol using ELF visibility if supported by the platform" ON)

if (SOCI_LTO)
    cmake_minimum_required(VERSION 3.9)

    # Check and enable lto support
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported)

    if (NOT supported)
        message(STATUS "IPO / LTO not supported")
    endif()

    if (supported AND NOT SOCI_ASAN)
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Check for lld as clang lto works best with its own linker
            soci_use_ld_if_supported(lld)
        endif()
    elseif(supported)
        message(STATUS "IPO / LTO is supported but conflicts with ASAN and not enabled")
    endif()
endif()

if (SOCI_VISIBILITY)
    # Test whether visibility is supported
    include(CheckCSourceCompiles)
    check_c_source_compiles(
        "
            __attribute__ (( visibility(\"default\") )) int f1() { return 0; }
            __attribute__ (( visibility(\"hidden\") ))  int f2() { return 1; }

            int main(int argc, char* argv[]) { f1(); f2(); return 0; }
        "
        SOCI_HAVE_VISIBILITY_SUPPORT
    )

    if (SOCI_HAVE_VISIBILITY_SUPPORT)
        message(STATUS "gcc / clang visibility enabled")
        set(CMAKE_CXX_VISIBILITY_PRESET hidden)
        cmake_policy(SET CMP0063 NEW)
    endif()
else()
    set(SOCI_HAVE_VISIBILITY_SUPPORT off)
endif()

# Allow using alternative linker such as mold, which can be significantly
# faster than GNU ld.
option(SOCI_LD "Use non-default linker, such as 'mold'" "")
if(SOCI_LD)
	# -fuse-ld works only with recent gcc (>= 12), but we don't need to support
	# this with all gcc versions as this is entirely optional. I.e. if we really
	# wanted to, we could use -B option as explained in mold README to make it
	# work with any gcc, but for now keep the things simple.
	soci_use_ld_if_supported(${SOCI_LD})
endif()

###############################################################################
# SOCI configuration
###############################################################################
include(SociConfig)

colormsg(_HIBLUE_ "Configuring SOCI:")

###############################################################################
# SOCI version information
###############################################################################
include(SociVersion)

soci_version()

###############################################################################
# Build features and variants
##############################################################################

boost_report_value(SOCI_SHARED)
boost_report_value(SOCI_STATIC)
boost_report_value(SOCI_TESTS)
boost_report_value(SOCI_ASAN)
boost_report_value(SOCI_UBSAN)

# from SociConfig.cmake
boost_report_value(LIB_SUFFIX)

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf
# directories. This simplifies manual testing and the use of the build
# tree rather than installed Boost libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###############################################################################
# Find SOCI dependencies
###############################################################################

set(SOCI_CORE_TARGET)
set(SOCI_CORE_TARGET_STATIC)
set(SOCI_CORE_DEPS_LIBS)

include(SociDependencies)

get_property(SOCI_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY INCLUDE_DIRECTORIES)

if(Threads_FOUND)
  list(APPEND SOCI_CORE_DEPS_LIBS ${CMAKE_THREAD_LIBS_INIT})
else()
  message(FATAL_ERROR "No thread library found")
endif()

if(NOT MSVC)
  set(DL_FIND_QUIETLY TRUE)
  find_package(DL)
  if(DL_FOUND)
    list(APPEND SOCI_CORE_DEPS_LIBS ${DL_LIBRARY})
    set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES ${DL_INCLUDE_DIR})
    add_definitions(-DHAVE_DL=1)
  endif()
endif()

if(Boost_FOUND)
  get_property(SOCI_COMPILE_DEFINITIONS
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY COMPILE_DEFINITIONS)

  set(SOCI_HAVE_BOOST ON)

  list(APPEND SOCI_COMPILE_DEFINITIONS "BOOST_ALL_NO_LIB")

  if(Boost_DATE_TIME_FOUND)
    list(APPEND SOCI_CORE_DEPS_LIBS ${Boost_DATE_TIME_LIBRARY})
    set(SOCI_HAVE_BOOST_DATE_TIME ON)
  endif()

  list(APPEND SOCI_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
  list(APPEND SOCI_CORE_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})

  set_directory_properties(PROPERTY COMPILE_DEFINITIONS "${SOCI_COMPILE_DEFINITIONS}")

  set_property(DIRECTORY ${SOCI_SOURCE_DIR}
    PROPERTY COMPILE_DEFINITIONS "${SOCI_COMPILE_DEFINITIONS}")
else()
  set(SOCI_HAVE_BOOST OFF)
  set(SOCI_HAVE_BOOST_DATE_TIME OFF)
endif()

set(SOCI_HAVE_BOOST ${SOCI_HAVE_BOOST} CACHE INTERNAL "Boost library")
set(SOCI_HAVE_BOOST_DATE_TIME ${SOCI_HAVE_BOOST_DATE_TIME} CACHE INTERNAL "Boost date_time library")

list(APPEND SOCI_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY
  INCLUDE_DIRECTORIES ${SOCI_INCLUDE_DIRS})

###############################################################################
# Installation
###############################################################################

include(GNUInstallDirs)

###############################################################################
# Configuration files
###############################################################################
set(CONFIG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
install(DIRECTORY ${CONFIG_INCLUDE_DIR}/soci DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(CONFIG_FILE_IN "include/soci/soci-config.h.in")
set(CONFIG_FILE_OUT "${CONFIG_INCLUDE_DIR}/soci/soci-config.h")



###############################################################################
# Build configured components
###############################################################################
include(SociBackend)

include_directories(${SOCI_SOURCE_DIR}/include ${CONFIG_INCLUDE_DIR})
add_subdirectory(src)

if(SOCI_TESTS)
  ###############################################################################
  # Enable tests
  ###############################################################################
  enable_testing()

  file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR} TEST_ACCESS_PATH)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/configs/test-access.cmake ${PROJECT_SOURCE_DIR}/tests/odbc/test-access.dsn @ONLY)

  set(MYSQL_DRIVER_NAME "MySQL")
  if(WIN32)
    set(MYSQL_DRIVER_NAME "MySQL ODBC 5.3 ANSI Driver")
  endif()
  configure_file(${PROJECT_SOURCE_DIR}/cmake/configs/test-mysql.cmake ${PROJECT_SOURCE_DIR}/tests/odbc/test-mysql.dsn @ONLY)

  # Define "make check" as alias for "make test"
  add_custom_target(check COMMAND ctest)
  add_subdirectory(tests)
endif()

###############################################################################
# platform-dependent settings
###############################################################################

include(CheckTypeSize)
check_type_size(long SOCI_SIZEOF_LONG)
set(SOCI_SIZEOF_LONG ${SOCI_SIZEOF_LONG} CACHE INTERNAL "Size of long")

include(CheckCXXSourceCompiles)

function(soci_check_types_are_same type1 type2)
    string(TOUPPER ${type1} type1_upper)
    string(MAKE_C_IDENTIFIER ${type1_upper} type1_id)
    string(TOUPPER ${type2} type2_upper)
    string(MAKE_C_IDENTIFIER ${type2_upper} type2_id)

    check_cxx_source_compiles("
#include <cstdint>

struct Foo {
    void foo(${type1} x);
    void foo(${type2} x);
};

int main() { return 0; }
"
        "SOCI_TYPES_${type1_id}_AND_${type2_id}_DIFFER"
    )

    if(SOCI_TYPES_${type1_id}_AND_${type2_id}_DIFFER)
        set(soci_types_equal FALSE)
    else()
        set(soci_types_equal TRUE)
    endif()

    set("SOCI_${type1_id}_IS_${type2_id}" ${soci_types_equal} CACHE INTERNAL "${type1} and ${type2} are the same type")
endfunction()

if(SOCI_SIZEOF_LONG EQUAL 8)
    # int64_t can be defined as either long or long long in this case, find out
    # which one we have.
    soci_check_types_are_same("int64_t" "long")
elseif(SOCI_SIZEOF_LONG EQUAL 4)
    # Nothing to do in this case, actually, as it's never going to be the same
    # as int32_t which is defined as int and not long in this case.
else()
    message(FATAL_ERROR "Unsupported size of long=${SOCI_SIZEOF_LONG}")
endif()

soci_check_types_are_same("int8_t" "char")

###############################################################################
# build config file
###############################################################################

get_cmake_property(ALL_VARIABLES CACHE_VARIABLES)
set(CONFIGURED_VARIABLES)
foreach(v ${ALL_VARIABLES})
    if (v MATCHES "^SOCI_(HAVE|[A-Z0-9_]+_IS)_")
	get_property(CACHE_HELPSTRING CACHE ${v} PROPERTY HELPSTRING)
		set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}\n// ${CACHE_HELPSTRING}\n")
        if (${${v}})
            set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}#define ${v}\n")
        else()
            set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}/* #undef ${v} */\n")
        endif()
    endif()

    if (v MATCHES "^SOCI_SIZEOF_")
	get_property(CACHE_HELPSTRING CACHE ${v} PROPERTY HELPSTRING)
		set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}\n// ${CACHE_HELPSTRING}\n")
        set(CONFIGURED_VARIABLES "${CONFIGURED_VARIABLES}#define ${v} ${${v}}\n")
    endif()
endforeach()
configure_file("${CONFIG_FILE_IN}" "${CONFIG_FILE_OUT}")

message(STATUS "")
