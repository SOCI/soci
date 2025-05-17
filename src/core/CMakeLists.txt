include(GNUInstallDirs)

set(SOCI_LIB_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}soci_" CACHE STRING "Specifies prefix for the lib directory")
set(SOCI_LIB_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}" CACHE STRING "Specifies suffix for the lib directory")
set(SOCI_DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}" CACHE STRING "Specifies suffix for the library file in debug mode")


find_package(Threads REQUIRED)

add_library(soci_core
  ${SOCI_LIB_TYPE}
    "backend-loader.cpp"
    "blob.cpp"
    "common.cpp"
    "connection-parameters.cpp"
    "connection-pool.cpp"
    "error.cpp"
    "into-type.cpp"
    "logger.cpp"
    "once-temp-type.cpp"
    "prepare-temp-type.cpp"
    "procedure.cpp"
    "ref-counted-prepare-info.cpp"
    "ref-counted-statement.cpp"
    "row.cpp"
    "rowid.cpp"
    "session.cpp"
    "soci-simple.cpp"
    "statement.cpp"
    "transaction.cpp"
    "unicode.cpp"
    "use-type.cpp"
    "values.cpp"
)

add_library(SOCI::Core ALIAS soci_core)

set_target_properties(
  soci_core PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME Core
)

# We have to explicitly list all public header files in order for them to get
# installed properly. This will automatically set the BASE_DIR up as an include
# directory for the target wrapped in a BUILD_INTERFACE generator expression.
# Note that we only add the general, public SOCI headers here. Backend-specific
# headers are added by the respective backend target.
target_sources(soci_core
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
    FILES
      "${PROJECT_SOURCE_DIR}/include/soci/backend-loader.h"
      "${PROJECT_SOURCE_DIR}/include/soci/bind-values.h"
      "${PROJECT_SOURCE_DIR}/include/soci/blob-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/blob.h"
      "${PROJECT_SOURCE_DIR}/include/soci/boost-fusion.h"
      "${PROJECT_SOURCE_DIR}/include/soci/boost-gregorian-date.h"
      "${PROJECT_SOURCE_DIR}/include/soci/boost-optional.h"
      "${PROJECT_SOURCE_DIR}/include/soci/boost-tuple.h"
      "${PROJECT_SOURCE_DIR}/include/soci/callbacks.h"
      "${PROJECT_SOURCE_DIR}/include/soci/column-info.h"
      "${PROJECT_SOURCE_DIR}/include/soci/connection-parameters.h"
      "${PROJECT_SOURCE_DIR}/include/soci/connection-pool.h"
      "${PROJECT_SOURCE_DIR}/include/soci/error.h"
      "${PROJECT_SOURCE_DIR}/include/soci/exchange-traits.h"
      "${PROJECT_SOURCE_DIR}/include/soci/fixed-size-ints.h"
      "${PROJECT_SOURCE_DIR}/include/soci/into-type.h"
      "${PROJECT_SOURCE_DIR}/include/soci/into.h"
      "${PROJECT_SOURCE_DIR}/include/soci/is-detected.h"
      "${PROJECT_SOURCE_DIR}/include/soci/log-context.h"
      "${PROJECT_SOURCE_DIR}/include/soci/logger.h"
      "${PROJECT_SOURCE_DIR}/include/soci/noreturn.h"
      "${PROJECT_SOURCE_DIR}/include/soci/once-temp-type.h"
      "${PROJECT_SOURCE_DIR}/include/soci/prepare-temp-type.h"
      "${PROJECT_SOURCE_DIR}/include/soci/procedure.h"
      "${PROJECT_SOURCE_DIR}/include/soci/query_transformation.h"
      "${PROJECT_SOURCE_DIR}/include/soci/ref-counted-prepare-info.h"
      "${PROJECT_SOURCE_DIR}/include/soci/ref-counted-statement.h"
      "${PROJECT_SOURCE_DIR}/include/soci/row-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/row.h"
      "${PROJECT_SOURCE_DIR}/include/soci/rowid-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/rowid.h"
      "${PROJECT_SOURCE_DIR}/include/soci/rowset.h"
      "${PROJECT_SOURCE_DIR}/include/soci/session.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-backend.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-platform.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-simple.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-types.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-unicode.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci.h"
      "${PROJECT_SOURCE_DIR}/include/soci/statement.h"
      "${PROJECT_SOURCE_DIR}/include/soci/std-optional.h"
      "${PROJECT_SOURCE_DIR}/include/soci/transaction.h"
      "${PROJECT_SOURCE_DIR}/include/soci/trivial-blob-backend.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-conversion-traits.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-conversion.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-holder.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-ptr.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-wrappers.h"
      "${PROJECT_SOURCE_DIR}/include/soci/use-type.h"
      "${PROJECT_SOURCE_DIR}/include/soci/use.h"
      "${PROJECT_SOURCE_DIR}/include/soci/values-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/values.h"
      "${PROJECT_SOURCE_DIR}/include/soci/version.h"
)

target_include_directories(soci_core
  PUBLIC
    "$<INSTALL_INTERFACE:${SOCI_INSTALL_INCLUDEDIR}>"
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/soci"
    "${PROJECT_SOURCE_DIR}/include/private"
)

if (SOCI_SHARED)
  target_compile_definitions(soci_core
    PUBLIC
      # Define the macro SOCI_DLL on Windows
      $<IF:$<PLATFORM_ID:Windows>,SOCI_DLL,>
  )
endif()

if (WITH_BOOST)
  # Try and find Boost with the date_time component.
  find_package(Boost COMPONENTS date_time QUIET)

  # Note that we shouldn't check Boost_FOUND here as it will be true even if
  # we found only Boost headers, but not the date_time library.
  if (TARGET Boost::date_time)
    set(SOCI_BOOST_DATE_TIME_MESSAGE " (with date_time component)")
    set(SOCI_DEPENDENCY_BOOST_COMPONENTS "date_time" CACHE INTERNAL "Boost components used by SOCI")

    target_link_libraries(soci_core PUBLIC Boost::date_time)
    target_compile_definitions(soci_core PUBLIC SOCI_HAVE_BOOST_DATE_TIME)
  else()
    # If we couldn't find Boost::date_time, retry searching for Boost headers
    # only and use only them, if found.
    if (WITH_BOOST STREQUAL "REQUIRED")
      find_package(Boost REQUIRED)
    else()
      find_package(Boost)
    endif()
  endif()

  # Here Boost_FOUND is true either if we had found date_time in the first
  # find_package() call or just Boost headers in the second one.
  if (Boost_FOUND)
    message(STATUS "Found Boost: v${Boost_VERSION_STRING} in ${Boost_INCLUDE_DIRS}${SOCI_BOOST_DATE_TIME_MESSAGE}")
    set(SOCI_DEPENDENCY_BOOST TRUE CACHE INTERNAL "Whether SOCI depends on Boost")

    target_link_libraries(soci_core PUBLIC Boost::boost Boost::disable_autolinking)
    target_compile_definitions(soci_core PUBLIC SOCI_HAVE_BOOST)
  endif()
endif()

target_link_libraries(soci_core
  PUBLIC
    $<BUILD_INTERFACE:soci_compiler_interface>
  PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if (DEFINED ABI_SUFFIX)
  set_target_properties(soci_core
    PROPERTIES
      OUTPUT_NAME "soci_core${ABI_SUFFIX}"
  )
endif()

cmake_path(
  ABSOLUTE_PATH SOCI_INSTALL_LIBDIR
  BASE_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
  OUTPUT_VARIABLE SOCI_INSTALL_FULL_LIBDIR
)

target_compile_definitions(soci_core
  PRIVATE
    DEFAULT_BACKENDS_PATH="${SOCI_INSTALL_FULL_LIBDIR}"
    SOCI_LIB_PREFIX="${SOCI_LIB_PREFIX}"
    SOCI_LIB_SUFFIX="${SOCI_LIB_SUFFIX}"
    SOCI_DEBUG_POSTFIX="${SOCI_DEBUG_POSTFIX}"
)
if (DEFINED ABI_VERSION)
  target_compile_definitions(soci_core
    PRIVATE
      SOCI_ABI_VERSION="${ABI_VERSION}"
  )
endif()


install(
  TARGETS soci_core
  EXPORT SOCICoreTargets
  RUNTIME DESTINATION "${SOCI_INSTALL_BINDIR}"
    COMPONENT soci_runtime
  LIBRARY DESTINATION "${SOCI_INSTALL_LIBDIR}"
    COMPONENT          soci_runtime
    NAMELINK_COMPONENT soci_development
  ARCHIVE DESTINATION "${SOCI_INSTALL_LIBDIR}"
    COMPONENT soci_development
  FILE_SET headers DESTINATION "${SOCI_INSTALL_INCLUDEDIR}"
    COMPONENT soci_development
)
# Generate and install a targets file
install(
  EXPORT SOCICoreTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCICoreTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
