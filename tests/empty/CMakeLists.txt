add_executable(soci_empty_test
  "test-empty.cpp"
)
target_link_libraries(soci_empty_test PRIVATE soci_tests_common SOCI::Empty)

set(SOCI_EMPTY_TEST_CONNSTR "dummy" CACHE STRING "The connection string to use for Empty tests")

add_test(
  NAME soci_empty_test
  COMMAND soci_empty_test "${SOCI_EMPTY_TEST_CONNSTR}" "--invisibles"
)
