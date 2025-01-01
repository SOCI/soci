
add_library(soci_tests_common
  STATIC
    "test-boost.cpp"
    "test-common.cpp"
    "test-connparams.cpp"
    "test-custom.cpp"
    "test-dynamic.cpp"
    "test-lob.cpp"
    "test-main.cpp"
    "test-manual.cpp"
    "test-rowset.cpp"
)

# Required to work around build issues with C++11 and Clang (see https://github.com/SOCI/soci/issues/984)
target_compile_definitions(soci_tests_common
  PUBLIC
    CATCH_CONFIG_CPP11_NO_SHUFFLE
)

target_link_libraries(soci_tests_common
  PUBLIC
    soci_compiler_interface
    SOCI::Core
)

target_include_directories(soci_tests_common
  PUBLIC
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/tests"
)
