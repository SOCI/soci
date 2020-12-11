# Conan package for SOCI

## Test the package creation
How to test the package from the path (`... soci/.conan/all`):

    conan create . <version>@
    conan create . 4.0.1@
    conan create . 4.0.0@

If the package creation was successful it will also run the tests for `sqlite3`

## How to use it
This recipe is meant to be tested only locally, but 
once the recipe is added to [conan.io/center](https://conan.io/center/),
the way to include SOCI in a new project using `cmake` will
be the following:

1. Create a `conanfile.txt` using `sqlite3` as backend: 
    
    ```
    [requires]
    sqlite3/3.33.0
    soci/4.0.1
    
    [options]
    soci:cxx11   = True
    soci:sqlite3 = True
    soci:empty   = True
    
    [generators]
    cmake
    ```

2. Add the following lines to the `CMakeLists.txt`:

    ```
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
    conan_target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
    ```

    The command `conan_target_link_libraries()` will replace the `target_link_libraries()`, so
    if you have any other library to link you can add it here instead, and remove
    the `target_link_libraries()`  
    The `${PROJECT_NAME}` is the name of the target.

3. Run from your build directory.  
    `conan install <path_to>/conanfile.txt` 

4. Build your project.

## Notes:

- To install conan use `python > 3`: `pip3 install --user conan`  
- Conan version used: `1.31.4`
- Package tested on: `Ubuntu 18.04`, `gcc-7`
- This Conan package supports `soci version > 4.0.0`
- This Conan package has these backends configured:
    - sqlite3

## Dependencies summary
 
|Option     | Library generated | has dependencies  | 
|:---:      | ---               | ---               |
|shared     | soci_core         | no                |
|empty      | soci_empty        | no                |
|static     |   ??              | ??                |
|cxx11      |   --              | y: `cxx_std > 11`       |
|sqlite3    | soci_sqlite       | y: `sqlite3 v 3.33.0`  |
|db2        |   N/A             | yes               |
|odbc       |   N/A             | yes               |
|oracle     | soci_oracle       | yes               |
|firebird   |   N/A             | yes               |
|mysql      | soci_mysql        | yes               |
|postgresql | soci_postgresql   | yes               |
|tests      |   ??              | ??                |
