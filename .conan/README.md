# Conan package for SOCI

## Test the package creation
How to test the package from the path (`... soci/.conan/all`):

    conan create . <version>@
    conan create . 4.0.1@

This recipe is meant to be tested only locally.

## How to use it

The recipe can be found in [conan.io/center](https://conan.io/center/) for
version `4.0.1`  

1. There are two ways to include `SOCI` in a new `cmake` project:
with a plain `conanfile.txt` or with a python `conanfile.py`.  

    1.1 Create a plain `conanfile.txt` to use `soci` with `sqlite3` as backend:  
    ```
    [requires]
      soci/4.0.1
    
    [options]
      soci:shared       = True
      soci:with_sqlite3 = True
    
    [generators]
      cmake
    ```

    1.2. Create a `conanfile.py` to use `soci` with `sqlite3` as backend:

    ```python
    import os
    from conans import ConanFile, CMake, tools
    
    class SomeProject(ConanFile):
        settings = "os", "compiler", "build_type", "arch"
        generators = "cmake"
    
        def configure(self):
            self.options["soci"].shared         = True
            self.options["soci"].with_sqlite3   = True
    
        def build(self):
            cmake = CMake(self)
            cmake.configure()
            cmake.build()
    ```

2. Add the following lines to the `CMakeLists.txt`:  
    ```cmake
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
    conan_target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
    ```

    - The command `conan_target_link_libraries()` will replace the `target_link_libraries()`, so
    if you have any other library to link you can add it here instead, and remove
    the `target_link_libraries()`  
    - The `${PROJECT_NAME}` is the name of the target.
    - The `${CONAN_LIBS}` is the collection of library names to be included. They
    can also be listed individually (`CONAN_PKG::soci_core`, `CONAN_PKG::sqlite3`)

3. Run the conan command from the build directory:  
    `conan install .. --build=soci` 

4. Include the required headers:  
    ```c++
    #include "soci/soci.h"
    #include "soci/sqlite3/soci-sqlite3.h"
    #include "sqlite3.h"
    ```

5. Configure and build your project as usual:  
    `cmake ..`  
    `cmake --build .`  

That's it.

## Notes:

- To install conan use `python3`: `pip3 install --user conan`  
- Conan version used: `1.31.4`
- Package tested on: `Ubuntu 18.04`, `gcc-7`
- Requires `soci version=4.0.1`
- Requires `cpp_standard >= 11`
- Backends configured:
    - [x] sqlite3
    - [x] odbc
    - [x] mysql
    - [x] postgresql
    - [ ] DB2
    - [ ] Oracle
    - [ ] firebird


## Dependencies summary
 
|Option     | Library generated | has dependencies  | 
|:---:      | ---               | ---               |
|shared     | soci_core         | no                |
|static     |   ??              | ??                |
|empty      | soci_empty        | no                |
|cxx11      |   --              | y: `cxx_std > 11`       |
|sqlite3    | soci_sqlite       | y: `sqlite3 v 3.33.0`  |
|db2        |   N/A             | yes               |
|odbc       |   N/A             | yes               |
|oracle     | soci_oracle       | yes               |
|firebird   |   N/A             | yes               |
|mysql      | soci_mysql        | yes               |
|postgresql | soci_postgresql   | yes               |
