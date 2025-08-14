# Installation

## Requirements

Below is an overall list of SOCI core:

* C++ compiler: [GCC](https://gcc.gnu.org/), [Microsoft Visual C++](https://msdn.microsoft.com/en-us/visualc), [LLVM/clang](https://clang.llvm.org/)
* [CMake](https://www.cmake.org) 3.23+
* Optional: [Boost C++ Libraries](https://www.boost.org): DateTime, Fusion, Optional, Preprocessor, Tuple

and backend-specific dependencies:

* [DB2 Call Level Interface (CLI)](https://pic.dhe.ibm.com/infocenter/db2luw/v10r1/topic/com.ibm.swg.im.dbclient.install.doc/doc/c0023452.html)
* [Firebird client library](https://www.firebirdsql.org/manual/ufb-cs-clientlib.html)
* [mysqlclient](https://dev.mysql.com/doc/refman/5.6/en/c-api.html) - C API to MySQL
* ODBC (Open Database Connectivity) implementation: [Microsoft ODBC](https://msdn.microsoft.com/en-us/library/windows/desktop/ms710252.aspx) [iODBC](https://www.iodbc.org/), [unixODBC](https://www.unixodbc.org/)
* [Oracle Call Interface (OCI)](https://www.oracle.com/technetwork/database/features/oci/index.html)
* [libpq](https://www.postgresql.org/docs/current/static/libpq.html) - C API to PostgreSQL
* [SQLite 3](https://www.sqlite.org/) library

## Downloads

Download package with latest release of the SOCI source code:
[soci-X.Y.Z](https://sourceforge.net/projects/soci/),
where X.Y.Z is the version number. Unpack the archive.

You can always clone SOCI from the Git repository:

```console
git clone --recurse-submodules git://github.com/SOCI/soci.git
```

## Building with CMake

For building SOCI, [CMake](https://cmake.org/) version 3.23 or later is
required.

The build configuration allows to control various aspects of compilation and
installation by setting common CMake variables that change behaviour, describe
system or control build (see [CMake help](https://cmake.org/cmake/help/documentation.html))
as well as SOCI-specific variables described below.
All these variables are available regardless of platform or compilation toolset used.

Running CMake from the command line allows to set variables in the CMake cache
with the following syntax: `-DVARIABLE:TYPE=VALUE`. If you are new to CMake,
you may find the tutorial [Running CMake](https://cmake.org/cmake/help/runningcmake.html) helpful.

### TL;DR

Steps outline using GNU Make `Makefile`-s:

```console
cmake -DSOCI_ORACLE=OFF (...) -B build -S /path/to/soci-X.Y.Z
cmake --build build
```

Optionally, SOCI defines an install target that can be executed (on Unix systems)
via `make install`.

### CMake configuration

By default, CMake will try to determine availability of all dependencies automatically.
If you are lucky, you will not need to specify any of the CMake variables explained below.
However, if CMake reports some of the core or backend-specific dependencies
as missing, you will need specify relevant variables to tell CMake where to look
for the required components.

The following sections provide summary of variables accepted by CMake scripts configuring SOCI build.
The lists consist of common variables for SOCI core and all backends as well as variables specific to SOCI backends and their direct dependencies.

SOCI build system respects the following standard CMake variables:

* `CMAKE_BUILD_TYPE` - string - Specifies the build type for make based generators (see CMake [help](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)).
* `CMAKE_INSTALL_PREFIX` - path - Install directory used by install command (see CMake [help](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html)).
* `CMAKE_VERBOSE_MAKEFILE` - boolean - If ON, create verbose makefile (see CMake [help](https://cmake.org/cmake/help/latest/variable/CMAKE_VERBOSE_MAKEFILE.html)).

The following SOCI-specific variables control common SOCI features and dependencies:

* `SOCI_SHARED` - boolean - Request to build shared libraries for SOCI. Default is `ON`.
* `SOCI_TESTS` - boolean - Request to build unit tests for SOCI. Default is `ON`, if you build SOCI standalone. Otherwise, it defaults to `OFF`.
* `WITH_BOOST` - string - Should CMake try to detect [Boost C++ Libraries](https://www.boost.org/). If `ON` (default), CMake will try to find Boost headers and binaries of [`Boost.Date_Time`](https://www.boost.org/doc/libs/release/doc/html/date_time.html) library and provide special support for Boost types if they are found. If `OFF`, no attempt to find Boost will be made. Finally, if this option has the value of `REQUIRED`, the behaviour is the same as with `ON` but an error is given if `Boost.Date_Time` is not found.

Some other build options:

* `SOCI_ASAN` - boolean - Build with address sanitizer (ASAN) support. Useful for finding problems when debugging, but shouldn't be used for the production builds due to extra overhead. Default is `OFF`.
* `SOCI_UBSAN` - boolean - Build with undefined behaviour sanitizer (ASAN) support. Default is `OFF`.
* `SOCI_LTO` - boolean - Build with link-time optimizations, if supported. This produces noticeably smaller libraries. Default is `OFF`, but turning it on is recommended for the production builds.
* `SOCI_NAME_PREFIX` and `SOCI_NAME_SUFFIX` - strings - If specified, the former is prepended and the latter appended to the names of all SOCI libraries. Note that by default SOCI build system appends `_MAJOR_MINOR` suffix to the names of Windows DLLs (but not to the names of Unix shared libraries) to prevent mixing up ABI-incompatible builds from different SOCI versions. If you want to override this behaviour, you can set `SOCI_NAME_SUFFIX` to an empty string.

When it comes to enabling specific backends, SOCI supports three distinct options that can be used as `Enabler` type as used below:

* `AUTO`: Try to locate the backend's dependencies. If all dependencies are met, enable the backend, otherwise disable it and continue without it.
* `OFF`: Disable the backend.
* `ON`: Enables the backend. If one or more of its dependencies are unmet, error and abort configuration.

SQLite backend is special, as it may use the built-in SQLite library if the system version is not found, see its documentation below for more details.

SOCI outputs the summary of its build configuration at the end of the CMake configuration step, so you can easily see which backends are effectively enabled. It is also possible to show the build configuration at any later time by running `cmake --build $builddir --target show_config` where `builddir` is the build directory.

#### Empty (sample backend)

* `SOCI_EMPTY` - Enabler - Enables the [sample backend](backends/index.md) called Empty. Always ON by default.
* `SOCI_EMPTY_TEST_CONNSTR` - string - Connection string used to run regression tests of the Empty backend. It is a dummy value. Example: `-DSOCI_EMPTY_TEST_CONNSTR="dummy connection"`
* `SOCI_EMPTY_SKIP_TESTS` - boolean - Skips testing this backend.

#### IBM DB2

* `SOCI_DB2` - Enabler - Enables the [DB2](backends/db2.md) backend.
* `DB2_INCLUDE_DIRS` - string - Path to DB2 CLI include directories where CMake should look for `sqlcli1.h` header.
* `DB2_LIBRARIES` - string - Full paths to  `db2` or `db2api` libraries to link SOCI against to enable the backend support.
* `SOCI_DB2_TEST_CONNSTR` - string - See [DB2 backend reference](backends/db2.md) for details. Example: `-DSOCI_DB2_TEST_CONNSTR:STRING="DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off"`
* `SOCI_DB2_SKIP_TESTS` - boolean - Skips testing this backend.

#### Firebird

* `SOCI_FIREBIRD` - Enabler - Enables the [Firebird](backends/firebird.md) backend.
* `Firebird_INCLUDE_DIRS` - string - Path to Firebird include directories where CMake should look for `ibase.h` header.
* `Firebird_LIBRARIES` - string - Full paths to Firebird `fbclient` or `fbclient_ms` libraries to link SOCI against to enable the backend support.
* `SOCI_FIREBIRD_TEST_CONNSTR` - string - See [Firebird backend reference](backends/firebird.md) for details. Example: `-DSOCI_FIREBIRD_TEST_CONNSTR:STRING="service=LOCALHOST:/tmp/soci_test.fdb user=SYSDBA password=masterkey"`
* `SOCI_FIREBIRD_SKIP_TESTS` - boolean - Skips testing this backend.

#### MySQL

* `SOCI_MYSQL` - Enabler - Enables the [MySQL](backends/mysql.md) backend.
* `MySQL_INCLUDE_DIRS` - string - Path to MySQL include directory where CMake should look for `mysql.h` header.
* `MySQL_LIBRARIES` - string - Full paths to libraries to link SOCI against to enable the backend support.
* `SOCI_MYSQL_TEST_CONNSTR` - string - Connection string to MySQL test database. Format of the string is explained [MySQL backend reference](backends/mysql.md). Example: `-DSOCI_MYSQL_TEST_CONNSTR:STRING="db=mydb user=mloskot password=secret"`
* `SOCI_MYSQL_SKIP_TESTS` - boolean - Skips testing this backend.

Furthermore, the `MYSQL_DIR` _environment variable_ can be set to the MySQL installation root. CMake will scan subdirectories `MYSQL_DIR/include` and `MYSQL_DIR/lib` respectively for MySQL headers and libraries.

#### ODBC

* `SOCI_ODBC` - Enabler - Enables the [ODBC](backends/odbc.md) backend.
* `SOCI_ODBC_TEST_{database}_CONNSTR` - string - ODBC Data Source Name (DSN) or ODBC File Data Source Name (FILEDSN) to test database: Microsoft Access (.mdb), Microsoft SQL Server, MySQL, PostgreSQL or any other ODBC SQL data source. {database} is placeholder for name of database driver ACCESS, MYSQL, POSTGRESQL, etc. See [ODBC](backends/odbc.md) backend reference for details. Example: `-DSOCI_ODBC_TEST_POSTGRESQL_CONNSTR="FILEDSN=/home/mloskot/soci/build/test-postgresql.dsn"`
* `SOCI_ODBC_SKIP_TESTS` - boolean - Skips testing this backend.

#### Oracle

* `SOCI_ORACLE` - Enabler - Enables the [Oracle](backends/oracle.md) backend.
* `Oracle_INCLUDE_DIRS` - string - Path to Oracle include directory where CMake should look for `oci.h` header.
* `Oracle_LIBRARIES` - string - Full paths to libraries to link SOCI against to enable the backend support.
* `SOCI_ORACLE_TEST_CONNSTR` - string - Connection string to Oracle test database. Format of the string is explained [Oracle backend reference](backends/oracle.md). Example: `-DSOCI_ORACLE_TEST_CONNSTR:STRING="service=orcl user=scott password=tiger"`
* `SOCI_ORACLE_SKIP_TESTS` - boolean - Skips testing this backend.

#### PostgreSQL

* `SOCI_POSTGRESQL` - Enabler - Enables the [PostgreSQL](backends/postgresql.md) backend.
* `SOCI_POSTGRESQL_TEST_CONNSTR` - string - Connection string to PostgreSQL test database. Format of the string is explained PostgreSQL backend reference. Example: `-DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="dbname=mydb user=scott"`
* `SOCI_POSTGRESQL_SKIP_TESTS` - boolean - Skips testing this backend.

#### SQLite 3

* `SOCI_SQLITE3` - Enabler - Enables the [SQLite3](backends/sqlite3.md) backend. Note that, unlike with all the other backends, if SQLite3 library is not found, built-in version of SQLite3 is used instead of the backend being disabled. `SOCI_SQLITE3_BUILTIN` can be set to `OFF` to prevent this from happening, i.e. force using system version only, or, on the contrary, set to `ON` to always use the built-in version, even if SQLite3 library is available on the system. In the latter case you may additionally set `SOCI_SQLITE3_DIRECTORY` to specify the directory containing `sqlite3.c` file which will be used during the build or `SOCI_SQLITE3_SOURCE_FILE` to specify the path to the `sqlite3.c` file directly.
* `SOCI_SQLITE3_TEST_CONNSTR` - string - Connection string is simply a file path where SQLite3 test database will be created (e.g. /home/john/soci_test.db). Check [SQLite3 backend reference](backends/sqlite3.md) for details. Example: `-DSOCI_SQLITE3_TEST_CONNSTR="my.db"` or `-DSOCI_SQLITE3_TEST_CONNSTR=":memory:"`.
* `SOCI_SQLITE3_SKIP_TESTS` - boolean - Skips testing this backend.

## Building with Conan

SOCI is available as a [Conan](https://docs.conan.io/en/latest/) package since
February 2021 for the version [4.0.1](https://conan.io/center/soci) for the
following backends: sqlite3, odbc, mysql, and postgresql.

This section lists the steps required to use SOCI in a CMake project with Conan:

### Install Conan

Install Conan if it is not installed yet:

```console
pip3 install conan
```

### Create `conanfile.txt`

Create a `conanfile.txt` in the same directory of the `CMakeLists.txt`, with a
_reference to a recipe_ (which is a string with the library name and the
version to use), the build options, and the CMake generator:

```text
# conanfile.txt
[requires]
soci/4.0.1

[options]
soci:shared       = True
soci:with_sqlite3 = True

[generators]
cmake
```

The option `soci:with_sqlite3 = True` indicates that the `sqlite3` backend will
be downloaded and used.

### Update `CMakeLists.txt`

Add the following Conan-specific lines to the `CMakeLists.txt` of your project:

```cmake
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
conan_target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
```

The command `conan_target_link_libraries` replaces `target_link_libraries`.

### Run Conan

Run `conan install` to install SOCI, and then build your project as usual:

```bash
mkdir build
cd build
conan install .. --build=soci
cmake ..
cmake . --build
```

## Running tests

The process of running regression tests highly depends on user's environment and build configuration, so it may be quite involving process.
The CMake configuration provides variables to allow users willing to run the tests to configure build and specify database connection parameters (see the lists above for variable names).

In order to run regression tests, configure and build desired SOCI backends and prepare working database instances for them.

While configuring build with CMake, specify `SOCI_TESTS=ON` to enable building regression tests.
Also, specify `SOCI_{backend name}_TEST_CONNSTR` variables to tell the tests runner how to connect with your test databases.

Dedicated `make test` target can be used to execute regression tests on build completion:

```console
mkdir build
cd build
cmake -G "Unix Makefiles" \
        -DWITH_BOOST=OFF \
        -DSOCI_TESTS=ON \
        -DSOCI_EMPTY_TEST_CONNSTR="dummy connection" \
        -DSOCI_SQLITE3_TEST_CONNSTR="test.db" \
        (...)
        ../soci-X.Y.Z
make
make test
make install
```

In the example above, regression tests for the sample Empty backend and SQLite 3 backend are configured for execution by `make test` target.

## Using the library

CMake build produces set separate libraries for SOCI's core and all enabled backends.

If your project also uses CMake, you can simply use `find_package(SOCI)` to check for SOCI availability and `target_link_libraries()` to link with the `SOCI::SOCI` target. An example of a very simple CMake-based project using SOCI is provided in the `examples/connect` directory.

Alternatively, you can add SOCI as a subdirectory to your project and include it via `add_subdirectory()`. As before, `target_link_libraries()` is used to link with the `SOCI::SOCI` target. An example of this can be found in the directory `examples/subdir-include`.

If you don't use CMake but want to use SOCI in your program, you need to specify the paths to the SOCI headers and libraries in your build configuration and to
tell the linker to link against the libraries you want to use in your program.
