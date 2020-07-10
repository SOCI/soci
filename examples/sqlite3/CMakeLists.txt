### Preamble ###
cmake_minimum_required(VERSION 3.17)
project(Test VERSION 0.1 LANGUAGES CXX)

##### Project wide setup ####
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(USE_FETCH OFF CACHE BOOL "Add boost support")

if(USE_FETCH)
    include(FetchContent)
    FetchContent_Declare(Soci
        GIT_REPOSITORY https://github.com/Leon0402/soci.git
        GIT_TAG        origin/feature/fetchExample
    )
    FetchContent_MakeAvailable(Soci)
else()
    find_package(Soci COMPONENTS Sqlite3 REQUIRED)
endif()

# Main build targets
add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_sqlite3)

add_subdirectory(src)