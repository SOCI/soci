# Note: we need >= v3.23 in order to make use of file sets for header installation
cmake_minimum_required(VERSION 3.23...3.31 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_modules")

include(soci_parse_version)
soci_parse_version(
  ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE SOCI_VERSION
)

project(SOCI
  VERSION ${SOCI_VERSION}
  DESCRIPTION "C++ database access library"
  HOMEPAGE_URL "https://soci.sourceforge.net/"
  LANGUAGES C CXX
)

include(soci_utils)

if (NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(CMakeDependentOption)
include(CheckIPOSupported)

check_ipo_supported(RESULT LTO_AVAILABLE)

if (SOCI_STATIC)
  set(SHARED_DEFAULT OFF)
elseif(DEFINED BUILD_SHARED_LIBS)
  set(SHARED_DEFAULT ${BUILD_SHARED_LIBS})
else()
  set(SHARED_DEFAULT ON)
endif()

option(SOCI_SHARED "Enable building SOCI as a shared library" ${SHARED_DEFAULT})
option(SOCI_TESTS "Enable building SOCI test cases" ${PROJECT_IS_TOP_LEVEL})
option(WITH_BOOST "Enable Boost support" ON)
cmake_dependent_option(SOCI_LTO "Enable link time optimizations in release builds" ON "LTO_AVAILABLE" OFF)

# Determine if the CMake generator is single or multi config. This will be
# useful later when printing the SOCI configuration in show_config.
get_property(SOCI_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Default build type to Release for single-config generators
if(NOT SOCI_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Configure LTO for anything but Debug builds (if enabled in the first place)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${SOCI_LTO})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

if (NOT APPLE)
  # This makes runtime loaders look for library dependencies
  # in the same directory as the library is located in.
  # For details see Craig Scott's CppCon 2019 talk

  # Note: The variable's content is set to $ORIGIN literally,
  # this is NOT a butchered cmake variable expansion
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  # Ensure that build artifacts are easy to find. And on Windows this
  # guarantees that the built DLLs end up next to applications
  # linking to them as otherwise they won't be found.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  if (NOT PROJECT_IS_TOP_LEVEL)
    # If the embedding project does not define these variables, this can lead to
    # inconsistencies which can cause issues on Windows when e.g. the embedding
    # project has an executable that links to a SOCI DLL which will be put into
    # a different directory which will lead to the exe not finding the DLL.
    message(WARNING "Setting CMAKE_{LIBRARY, ARCHIVE, RUNTIME}_DIRECTORY variables which should have done by the embedding cmake project")
  endif()
endif()

if (SOCI_SHARED)
  set(SOCI_LIB_TYPE "SHARED")
else()
  set(SOCI_LIB_TYPE "STATIC")
endif()

# When using shared libraries, use version-dependent suffix in their names.
if (SOCI_SHARED)
  if (WIN32)
    # Use the full version number as ABI version on Windows, different minor
    # versions are not ABI-compatible.
    set(ABI_VERSION "${PROJECT_VERSION_MAJOR}_${PROJECT_VERSION_MINOR}")

    # Also use it in the library name by default to ensure that the DLLs of
    # different versions are called differently as there is no SOVERSION on
    # Windows to prevent this from happening.
    set(soci_default_suffix "_${ABI_VERSION}")
  elseif(UNIX)
    # Use same value as for SOVERSION, which is only the major version (and is
    # used automatically to construct the shared libraries names, so there is
    # no need for any suffix).
    set(ABI_VERSION "${PROJECT_VERSION_MAJOR}")
  endif()
endif()

set(SOCI_NAME_PREFIX "" CACHE STRING "Optional prefix to add before 'soci_' in library names")
set(SOCI_NAME_SUFFIX "${soci_default_suffix}" CACHE STRING "Optional suffix to add to the library names")

# By default, we use system fmt if available, otherwise the bundled one, but
# this option can be used to either force or forbid using the bundled one.
set(SOCI_FMT_BUILTIN "" CACHE STRING "Prefer, or forbid, using the bundled fmt library")

set(soci_use_bundled_fmt ON)
if(NOT SOCI_FMT_BUILTIN)
  find_package(fmt QUIET)
  if (fmt_FOUND)
    message(STATUS "Using system fmt library ${fmt_VERSION}")
    set(soci_use_bundled_fmt OFF)
  else()
    if ("${SOCI_FMT_BUILTIN}" STREQUAL "OFF")
      message(
        FATAL_ERROR
          "System fmt library not found and using built-in version was disabled."
      )
    endif()
  endif()
endif()

# Name of the library to link with.
set(soci_fmt fmt::fmt)

if(soci_use_bundled_fmt)
  if (NOT EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/fmt/CMakeLists.txt)
    message(
      FATAL_ERROR
        "fmt library submodule not found.\n"
        "Please ensure that you have cloned the SOCI repository with --recurse-submodules."
    )
  endif()

  add_subdirectory(3rdparty/fmt EXCLUDE_FROM_ALL)
  if (NOT SOCI_SHARED)
    # When building SOCI as static library, we can't depend on fmt library, as
    # this would require SOCI users to link with fmt too, which would also
    # require installing it and we don't want to do this, so use header-only
    # library in this case and also use only its build interface, as otherwise
    # we'd still have a dependency on it in the SOCI targets.
    set(soci_fmt $<BUILD_INTERFACE:fmt::fmt-header-only>)
  endif()
endif()

# Helper function appending prefix and suffix to the library name.
function(soci_build_library_name result basename)
  set(${result} "${SOCI_NAME_PREFIX}${basename}${SOCI_NAME_SUFFIX}" PARENT_SCOPE)
endfunction()

include(soci_compiler_options)
include(soci_install_dirs)

add_subdirectory(src)

include(soci_compat)


if (SOCI_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()


# Packaging
include(CMakePackageConfigHelpers)
configure_package_config_file("soci-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/soci-config.cmake"
  INSTALL_DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(soci-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/soci-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/soci-config-version.cmake"
    DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
)

# Give build summary at the end but only if it's the first time we're running,
# as indicated by SOCI_SUMMARY not being cached yet, or if it has changed.
set(SOCI_SUMMARY_NOW
  "${SOCI_VERSION}-${SOCI_LIB_TYPE}-${SOCI_ENABLED_BACKENDS}-${CMAKE_BUILD_TYPE}"
)
if(NOT "${SOCI_SUMMARY_NOW}" STREQUAL "${SOCI_SUMMARY}")
set(SOCI_SUMMARY ${SOCI_SUMMARY_NOW} CACHE INTERNAL "SOCI internal build summary")

if (CMAKE_BUILD_TYPE)
  set(SOCI_CONFIG_DESCRIPTION " in ${CMAKE_BUILD_TYPE} configuration")
endif()
message(STATUS "SOCI ${SOCI_VERSION} will be built as ${SOCI_LIB_TYPE} library${SOCI_CONFIG_DESCRIPTION}.")
message(STATUS "Enabled SOCI backends are: ${SOCI_ENABLED_BACKENDS}")

endif()

# Also make it possible to see SOCI configuration later.
add_custom_target(show_config
  COMMAND ${CMAKE_COMMAND} -E echo
    "SOCI ${SOCI_VERSION} configured with:"
  COMMAND ${CMAKE_COMMAND} -E echo
    # If using multi-config generator CMAKE_BUILD_TYPE is not meaningful. One
    # would instead select a build type from CMAKE_CONFIGURATION_TYPES.
    "  Build type:       $<IF:${SOCI_IS_MULTI_CONFIG},${CMAKE_CONFIGURATION_TYPES},${CMAKE_BUILD_TYPE}>"
  COMMAND ${CMAKE_COMMAND} -E echo
    "  Library type:     ${SOCI_LIB_TYPE}"
  COMMAND ${CMAKE_COMMAND} -E echo
    "  Enabled backends: ${SOCI_ENABLED_BACKENDS}"
  COMMENT
    "Show SOCI build configuration summary"
  VERBATIM
)
