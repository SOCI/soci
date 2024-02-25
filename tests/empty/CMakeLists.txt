add_executable(soci_empty_tests empty_tests.cpp)
target_link_libraries(soci_empty_tests PRIVATE SOCI::Empty)
target_include_directories(soci_empty_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")

add_test(
  NAME soci_empty_tests
  COMMAND soci_empty_tests "--invisibles"
)
