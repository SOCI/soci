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
file(GLOB SOCI_HEADER_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/soci/*.h*")
target_sources(soci_core
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
    FILES "${SOCI_HEADER_FILES}"
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

if (WIN32)
  set(ABI_VERSION "${PROJECT_VERSION_MAJOR}_${PROJECT_VERSION_MINOR}")
elseif(UNIX)
  # Use SOVERSION, which is only the major version
  set(ABI_VERSION "${PROJECT_VERSION_MAJOR}")
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
