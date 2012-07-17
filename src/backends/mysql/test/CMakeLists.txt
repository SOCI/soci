

soci_backend_test(
  BACKEND MySQL
  SOURCE test-mysql.cpp
  CONNSTR "dummy")

add_custom_command(TARGET soci_mysql_test POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy ${mysql_DLL} ${CMAKE_CURRENT_BINARY_DIR} )