add_library(empty_tests OBJECT empty_tests.cpp)
target_link_libraries(empty_tests PUBLIC soci_empty_interface)
target_include_directories(empty_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")

soci_make_tests(
  OBJECT_LIB        empty_tests
  CONNECTION_STRING "dummy"
  SHARED_NAME       "soci_empty_test"
  STATIC_NAME       "soci_empty_test_static"
  SOCI_DEP_ALIAS    "Empty"
)
