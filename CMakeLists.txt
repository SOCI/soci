cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

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
  LANGUAGES CXX
)

include(soci_utils)

if (NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED OR CMAKE_CXX_STANDARD LESS 14)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CMakeDependentOption)
include(CheckIPOSupported)
include(CheckCXXCompilerFlag)

check_ipo_supported(RESULT LTO_AVAILABLE)

if (SOCI_STATIC)
  set(SHARED_DEFAULT OFF)
else()
  set(SHARED_DEFAULT ON)
endif()

option(SOCI_SHARED "Enable building SOCI as a shared library" ${SHARED_DEFAULT})
option(SOCI_TESTS "Enable building SOCI test cases" ${PROJECT_IS_TOP_LEVEL})
option(SOCI_ASAN "Enable building SOCI with enabled address sanitizers" OFF)
cmake_dependent_option(SOCI_LTO "Enable link time optimizations in release builds" ON "LTO_AVAILABLE" OFF)
option(SOCI_VISIBILITY "Make all functions hidden by default - this exposes only explicitly exported functions" ON)
set(SOCI_LD "" CACHE STRING "Specify a non-default linker")
if (WITH_BOOST)
  message(DEPRECATION "Use of WITH_BOOST option is deprecated - use SOCI_BOOST instead")
  option(SOCI_BOOST "Whether to enable Boost-specific bindings" ${WITH_BOOST})
else()
  # Default-enable only if Boost has been found before (in a superproject)
  option(SOCI_BOOST "Whether to enable Boost-specific bindings" ${Boost_FOUND})
endif()


# Configure LTO for anything but Debug builds (if enabled in the first place)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${SOCI_LTO})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_Debug OFF)

if (SOCI_VISIBILITY)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

if (NOT APPLE)
  # This makes runtime loaders look for library dependencies
  # in the same directory as the library is located in.
  # For details see Craig Scott's CppCon 2019 talk

  # Note: The variable's content is set to $ORIGIN literally,
  # this is NOT a butchered cmake variable expansion
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

if (SOCI_LD)
  # CMake asks the compiler to do the linking so we have to pass the desired linker to the compiler
  set(USE_LD_FLAG "-fuse-ld=${SOCI_LD}")
  check_cxx_compiler_flag("${USE_LD_FLAG}" CAN_USE_CUSTOM_LD)
  if (NOT CAN_USE_CUSTOM_LD)
    message(FATAL_ERROR "Can't use custom linker '${SOCI_LD}' - compiler doesn't accept flag '${USE_LD_FLAG}'")
  endif()
  add_link_options("${USE_LD_FLAG}")
endif()

# Ensure that libraries and executables end up at the top of the build tree
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (SOCI_SHARED)
  set(SOCI_LIB_TYPE "SHARED")
else()
  set(SOCI_LIB_TYPE "STATIC")
endif()


set(SOCI_LIB_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}soci_" CACHE STRING "Specifies prefix for the lib directory")
set(SOCI_LIB_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}" CACHE STRING "Specifies suffix for the lib directory")
set(SOCI_DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}" CACHE STRING "Specifies suffix for the library file in debug mode")


include(soci_compiler_options)

include(GNUInstallDirs)

set(SOCI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/soci-${PROJECT_VERSION}")

add_subdirectory(src)

message(STATUS "This: ${SOCI_DEPENDENCY_SOCI_COMPONENTS}")

include(soci_compat)


if (SOCI_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()


# Packaging
include(CMakePackageConfigHelpers)
configure_package_config_file("SOCIConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/SOCIConfig.cmake"
  INSTALL_DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(SOCIConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SOCIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SOCIConfigVersion.cmake"
    DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
)
