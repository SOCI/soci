
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
    "test-unicode.cpp"
)

# Catch headers are sensitive when it comes to being included in different
# contexts and this can lead to issues with all functionality to actually
# running the tests i.e. the main function.
# Therefore, we have to make sure that the main file is not included in
# a unity build.
set_source_files_properties("test-main.cpp"
  PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION TRUE
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

if (SOCI_SHARED)
  # We have a test checking for loading backend dynamically which can only be
  # done when using shared libraries, so define a symbol to guard it.
  target_compile_definitions(soci_tests_common PRIVATE SOCI_DYNAMIC_BACKEND)
endif()
