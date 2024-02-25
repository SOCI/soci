find_package(Threads REQUIRED)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
  "
      __attribute__ (( visibility(\"default\") )) int f1() { return 0; }
      __attribute__ (( visibility(\"hidden\") ))  int f2() { return 1; }

      int main(int argc, char* argv[]) { f1(); f2(); return 0; }
  "
  SOCI_VISIBILITY_ATTRIBUTE_SUPPORTED
)

# TODO: Actually populate this config file with something useful
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/soci-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/soci/soci-config.h")

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
    "use-type.cpp"
    "values.cpp"
)
add_library(SOCI::Core ALIAS soci_core)

target_include_directories(soci_core
  PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/include"
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

if (SOCI_SHARED AND SOCI_VISIBILITY AND SOCI_VISIBILITY_ATTRIBUTE_SUPPORTED)
  target_compile_definitions(soci_core
    PUBLIC
      SOCI_HAVE_VISIBILITY_SUPPORT
  )
endif()

if (SOCI_BOOST)
  find_package(Boost REQUIRED)
  target_link_libraries(soci_core PUBLIC Boost::boost)
  target_compile_definitions(soci_core PUBLIC SOCI_HAVE_BOOST)

  if (TARGET Boost::date_time)
    target_link_libraries(soci_core PUBLIC Boost::date_time)
    target_compile_definitions(soci_core PUBLIC SOCI_HAVE_BOOST_DATE_TIME)
  endif()
endif()

target_link_libraries(soci_core
  PRIVATE
    Threads::Threads
)

target_compile_definitions(soci_core
  PRIVATE
    DEFAULT_BACKENDS_PATH="${CMAKE_INSTALL_FULL_LIBDIR}"
    # TODO: Configure prefix and suffix properly
    SOCI_LIB_PREFIX=""
    SOCI_LIB_SUFFIX=""
)

