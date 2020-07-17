# This example shows how to use soci with sqlite3. 
# It shows two common approaches, one is to use findPackage if you have built soci and installed it on your system. 
# The other is adding soci as a subdirectory with FetchContent and build it as part of this project. 
# The default is findPackage, you can use fetchContent by providing -DUSE_FETCH=ON on the command line.

### Preamble ###
cmake_minimum_required(VERSION 3.17)
project(Test VERSION 0.1 LANGUAGES CXX)

##### Project wide setup ####
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_FETCH "Use fetchContent instead of findPackage" OFF)

### Dependencies ###
add_subdirectory(dependencies)
if(NOT USE_FETCH)
    find_package(Soci COMPONENTS Oracle REQUIRED)
endif()

### Main Targets ###
add_subdirectory(src)
