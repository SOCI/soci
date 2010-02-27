colormsg(_HIBLUE_ "Configuring SOCI backends:")

add_subdirectory(empty)
add_subdirectory(mysql)
add_subdirectory(postgresql)
add_subdirectory(sqlite3)

message(STATUS "")
