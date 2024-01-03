find_package(Threads REQUIRED)

add_library(soci_core_objects OBJECT
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

# TODO: Actually populate this config file with something useful
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/soci-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/soci/soci-config.h")
target_include_directories(soci_core_objects PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(soci_core_objects PRIVATE Threads::Threads)

if (SOCI_BOOST)
  find_package(Boost REQUIRED)
  target_link_libraries(soci_core_objects PUBLIC Boost::boost)
  target_compile_definitions(soci_core_objects PUBLIC SOCI_HAVE_BOOST)

  if (TARGET Boost::date_time)
    target_link_libraries(soci_core_objects PUBLIC Boost::date_time)
    target_compile_definitions(soci_core_objects PUBLIC SOCI_HAVE_BOOST_DATE_TIME)
  endif()
endif()

target_include_directories(soci_core_objects PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(soci_core_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/soci"
    "${PROJECT_SOURCE_DIR}/include/private"
)

target_compile_definitions(soci_core_objects
  PRIVATE
    DEFAULT_BACKENDS_PATH="${CMAKE_INSTALL_FULL_LIBDIR}"
    # TODO: Configure prefix and suffix properly
    SOCI_LIB_PREFIX=""
    SOCI_LIB_SUFFIX=""
)


if (SOCI_SHARED)
  add_library(soci_core SHARED)
  target_link_libraries(soci_core PUBLIC soci_core_objects)
  add_library(SOCI::shared::core ALIAS soci_core)
endif()
if (SOCI_STATIC)
  add_library(soci_core_static STATIC)
  target_link_libraries(soci_core_static PUBLIC soci_core_objects)
  add_library(SOCI::static::core ALIAS soci_core_static)
endif()
