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

target_include_directories(soci_core
  PUBLIC
    "${SOCI_GENERATED_INCLUDES_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
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
    soci_compiler_interface
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
