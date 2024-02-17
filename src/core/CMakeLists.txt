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

add_library(soci_core_interface INTERFACE)
target_include_directories(soci_core_interface
  INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include"
)
target_compile_definitions(soci_core_interface
  INTERFACE
    # Define the macro SOCI_DLL on Windows
    $<IF:$<PLATFORM_ID:Windows>,SOCI_DLL,>
)

if (SOCI_VISIBILITY AND SOCI_VISIBILITY_ATTRIBUTE_SUPPORTED)
  target_compile_definitions(soci_core_interface
    INTERFACE
      SOCI_HAVE_VISIBILITY_SUPPORT
  )
endif()

if (SOCI_BOOST)
  find_package(Boost REQUIRED)
  target_link_libraries(soci_core_interface INTERFACE Boost::boost)
  target_compile_definitions(soci_core_interface INTERFACE SOCI_HAVE_BOOST)

  if (TARGET Boost::date_time)
    target_link_libraries(soci_core_interface INTERFACE Boost::date_time)
    target_compile_definitions(soci_core_interface INTERFACE SOCI_HAVE_BOOST_DATE_TIME)
  endif()
endif()

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

target_link_libraries(soci_core_objects
  PUBLIC
    soci_core_interface
  PRIVATE
    Threads::Threads
)

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
  add_library(SOCI::shared::Core ALIAS soci_core)
  target_link_libraries(soci_shared_interface INTERFACE SOCI::shared::core)
endif()
if (SOCI_STATIC)
  add_library(soci_core_static STATIC)
  target_link_libraries(soci_core_static PUBLIC soci_core_objects)
  add_library(SOCI::static::Core ALIAS soci_core_static)
  target_link_libraries(soci_static_interface INTERFACE SOCI::static::core)
endif()
