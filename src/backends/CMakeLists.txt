message(STATUS "")
colormsg(_HIBLUE_ "Configuring SOCI database backends:")

# TODO: Scan and add directories automatically --mloskot
add_subdirectory(empty)
add_subdirectory(mysql)
add_subdirectory(odbc)
add_subdirectory(oracle)
add_subdirectory(postgresql)
add_subdirectory(sqlite3)

message(STATUS "")
