add_executable(soci_empty_tests
  "test-empty.cpp"
   # We only run these tests from the empty backend test, as they don't use
   # database at all.
   "../common/test-unicode.cpp"
)
target_link_libraries(soci_empty_tests PRIVATE soci_tests_common SOCI::Empty)

set(SOCI_EMPTY_TEST_CONNSTR "dummy" CACHE STRING "The connection string to use for Empty tests")

add_test(
  NAME soci_empty_tests
  COMMAND soci_empty_tests "${SOCI_EMPTY_TEST_CONNSTR}" "--invisibles"
)
