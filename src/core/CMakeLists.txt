include(GNUInstallDirs)

find_package(Threads REQUIRED)

add_library(soci_core
  ${SOCI_LIB_TYPE}
    "backend-loader.cpp"
    "blob.cpp"
    "common.cpp"
    "callbacks.cpp"
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
    "use-type.cpp"
    "values.cpp"
)

add_library(SOCI::Core ALIAS soci_core)

# We have to explicitly list all public header files in order for them to get
# installed properly. This will automatically set the BASE_DIR up as an include
# directory for the target wrapped in a BUILD_INTERFACE generator expression.
target_sources(soci_core
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
    FILES
      "${PROJECT_SOURCE_DIR}/include/soci/backend-loader.h"
      "${PROJECT_SOURCE_DIR}/include/soci/bind-values.h"
      "${PROJECT_SOURCE_DIR}/include/soci/blob-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/blob.h"
      "$<$<BOOL:${SOCI_HAVE_BOOST}>:${PROJECT_SOURCE_DIR}/include/soci/boost-fusion.h>"
      "$<$<BOOL:${SOCI_HAVE_BOOST_DATE_TIME}>:${PROJECT_SOURCE_DIR}/include/soci/boost-gregorian-date.h>"
      "$<$<BOOL:${SOCI_BOOST}>:${PROJECT_SOURCE_DIR}/include/soci/boost-optional.h>"
      "$<$<BOOL:${SOCI_BOOST}>:${PROJECT_SOURCE_DIR}/include/soci/boost-tuple.h>"
      "${PROJECT_SOURCE_DIR}/include/soci/callbacks.h"
      "${PROJECT_SOURCE_DIR}/include/soci/column-info.h"
      "${PROJECT_SOURCE_DIR}/include/soci/connection-parameters.h"
      "${PROJECT_SOURCE_DIR}/include/soci/connection-pool.h"
      "${PROJECT_SOURCE_DIR}/include/soci/error.h"
      "${PROJECT_SOURCE_DIR}/include/soci/exchange-traits.h"
      "${PROJECT_SOURCE_DIR}/include/soci/fixed-size-ints.h"
      "${PROJECT_SOURCE_DIR}/include/soci/into.h"
      "${PROJECT_SOURCE_DIR}/include/soci/into-type.h"
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
      "${PROJECT_SOURCE_DIR}/include/soci/soci.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-platform.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-simple.h"
      "${PROJECT_SOURCE_DIR}/include/soci/soci-types.h"
      "${PROJECT_SOURCE_DIR}/include/soci/statement.h"
      "${PROJECT_SOURCE_DIR}/include/soci/std-optional.h"
      "${PROJECT_SOURCE_DIR}/include/soci/transaction.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-conversion.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-conversion-traits.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-holder.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-ptr.h"
      "${PROJECT_SOURCE_DIR}/include/soci/type-wrappers.h"
      "${PROJECT_SOURCE_DIR}/include/soci/use.h"
      "${PROJECT_SOURCE_DIR}/include/soci/use-type.h"
      "${PROJECT_SOURCE_DIR}/include/soci/values-exchange.h"
      "${PROJECT_SOURCE_DIR}/include/soci/values.h"
      "${PROJECT_SOURCE_DIR}/include/soci/version.h"
)

target_include_directories(soci_core
  PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
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

if (SOCI_BOOST)
  find_package(Boost REQUIRED)
  target_link_libraries(soci_core PUBLIC Boost::boost)
  set(SOCI_HAVE_BOOST TRUE CACHE INTERNAL "" FORCE)

  find_package(Boost COMPONENTS date_time)

  if (TARGET Boost::date_time)
    target_link_libraries(soci_core PUBLIC Boost::date_time)
    set(SOCI_HAVE_BOOST_DATE_TIME TRUE CACHE INTERNAL "" FORCE)
  endif()
endif()

target_link_libraries(soci_core
  PUBLIC
    $<BUILD_INTERFACE:soci_compiler_interface>
  PRIVATE
    Threads::Threads
)

if (WIN32)
  set(VERSION_SEP "_")
elseif(UNIX)
  set(VERSION_SEP ".")
else()
  message(WARNING "No known ABI version scheme for current platform")
  set(VERSION_SEP "-")
endif()

target_compile_definitions(soci_core
  PRIVATE
    DEFAULT_BACKENDS_PATH="${CMAKE_INSTALL_FULL_LIBDIR}"
    SOCI_LIB_PREFIX="${SOCI_LIB_PREFIX}"
    SOCI_LIB_SUFFIX="${SOCI_LIB_SUFFIX}"
    SOCI_DEBUG_SUFFIX="${SOCI_DEBUG_SUFFIX}"
    SOCI_ABI_VERSION="${PROJECT_VERSION_MAJOR}${VERSION_SEP}${PROJECT_VERSION_MINOR}"
)

set_target_properties(soci_core PROPERTIES EXPORT_NAME Core)

install(
  TARGETS soci_core
  EXPORT SociCoreTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/soci"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/soci"
  FILE_SET headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/soci"
)
# Generate and install the export file for the target
install(
  EXPORT SociCoreTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/soci"
  FILE SociCoreTargets.cmake
  NAMESPACE SOCI::
)
