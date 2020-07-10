### Preamble ###
cmake_minimum_required(VERSION 3.17)
project(Test VERSION 0.1 LANGUAGES CXX)

##### Project wide setup ####
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Soci COMPONENTS Sqlite3 REQUIRED)

# Main build targets
add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_sqlite3)

add_subdirectory(src)