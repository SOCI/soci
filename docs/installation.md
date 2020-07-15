# Installation

## Requirements

Below is an overall list of SOCI core:

* C++ compiler: [GCC](http://gcc.gnu.org/), [Microsoft Visual C++](http://msdn.microsoft.com/en-us/visualc), [LLVM/clang](http://clang.llvm.org/)
* [CMake](http://www.cmake.org) 3.17+ - in order to use build configuration for CMake
* [Boost C++ Libraries](http://www.boost.org): DateTime, Fusion, Optional, Preprocessor, Tuple

and backend-specific dependencies:

* [DB2 Call Level Interface (CLI)](http://pic.dhe.ibm.com/infocenter/db2luw/v10r1/topic/com.ibm.swg.im.dbclient.install.doc/doc/c0023452.html)
* [Firebird client library](http://www.firebirdsql.org/manual/ufb-cs-clientlib.html)
* [mysqlclient](https://dev.mysql.com/doc/refman/5.6/en/c-api.html) - C API to MySQL
* ODBC (Open Database Connectivity) implementation: [Microsoft ODBC](http://msdn.microsoft.com/en-us/library/windows/desktop/ms710252.aspx) [iODBC](http://www.iodbc.org/), [unixODBC](http://www.unixodbc.org/)
* [Oracle Call Interface (OCI)](http://www.oracle.com/technetwork/database/features/oci/index.html)
* [libpq](http://www.postgresql.org/docs/current/static/libpq.html) - C API to PostgreSQL
* [SQLite 3](http://www.sqlite.org/) library

## Downloads

Download package with latest release of the SOCI source code:
[soci-X.Y.Z](https://sourceforge.net/projects/soci/),
where X.Y.Z is the version number. Unpack the archive.

You can always clone SOCI from the Git repository:

```console
git clone git://github.com/SOCI/soci.git
```

## Building with CMake

SOCI is configured to build using [CMake](http://cmake.org/) system in version 3.17+.

The build configuration allows to control various aspects of compilation and
installation by setting common CMake variables that change behaviour, describe
system or control build (see [CMake help](http://cmake.org/cmake/help/documentation.html))
as well as SOCI-specific variables described below.
All these variables are available regardless of platform or compilation toolset used.

Running CMake from the command line allows to set variables in the CMake cache
with the following syntax: `-DVARIABLE:TYPE=VALUE`. If you are new to CMake,
you may find the tutorial [Running CMake](http://cmake.org/cmake/help/runningcmake.html) helpful.

### Running CMake (any platform)

First configure cmake and create a native buildsystem (e.g. make on Unix or ninja, visual studio on Windows ...)
```console
cmake -S . - build  
```
The above command will just build soci_core as a static lib. Building soci core and the sqlite3 backend as a shared lib without tests looks like this:
```console
cmake -S . - build  -DSOCI_WITH_SQLITE3=ON -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF
```
Take a look at the following sections to read more about the cached variables for configuration.


Next build soci, backends, tests (depending on what you configured):
```console
cmake --build build
```

Finally install the built libs and cmake configuration files. You presumably need admin rights for this command:
```console
cmake --build build --target install
```

### CMake configuration

By default, CMake will try to determine availability of all dependencies automatically.
If you are lucky, you will not need to specify any of the CMake variables explained below.
However, if CMake reports some of the core or backend-specific dependencies
as missing, you will need specify relevant variables to tell CMake where to look
for the required components.

CMake configures SOCI build performing sequence of steps.
Each subsequent step is dependant on result of previous steps corresponding with particular feature.
First, CMake checks system platform and compilation toolset.
Next, CMake tries to find all external dependencies.
Then, depending on the results of the dependency check, CMake determines SOCI backends which are possible to build.
The SOCI-specific variables described below provide users with basic control of this behaviour.

The following sections provide summary of variables accepted by CMake scripts configuring SOCI build.
The lists consist of common variables for SOCI core and all backends as well as variables specific to SOCI backends and their direct dependencies.

List of a few essential CMake variables:

* `CMAKE_BUILD_TYPE` - string - Specifies the build type for make based generators (see CMake [help](http://cmake.org/cmake/help/cmake-2-8-docs.html#variable:CMAKE_BUILD_TYPE)).
* `CMAKE_INSTALL_PREFIX` - path - Install directory used by install command (see CMake [help](http://cmake.org/cmake/help/cmake-2-8-docs.html#variable:CMAKE_INSTALL_PREFIX)).
* `CMAKE_VERBOSE_MAKEFILE` - boolean - If ON, create verbose makefile (see CMake [help](http://cmake.org/cmake/help/cmake-2-8-docs.html#variable:CMAKE_VERBOSE_MAKEFILE)).

List of variables to control common SOCI features and dependencies:

* `BUILD_SHARED_LIBS` - boolean - OFF - Requests to build shared libraries for SOCI core and all successfully configured backends. Default is `OFF`.
* `BUILD_TESTING` - boolean - ON - Requests to build regression tests for SOCI core and all successfully configured backends.
* `SOCI_WITH_CXX11` - boolean - OFF - Request to compile in C++11 compatibility mode.
* `SOCI_WITH_BOOST` - boolean - OFF - Requests to build with boost type support.

#### Empty (sample backend)

* `SOCI_WITH_EMPTY` - boolean - ON - Requests to build the [sample backend](backends/index.md) called Empty.
* `SOCI_EMPTY_TEST_CONNSTR` - string - :memory: - Connection string used to run regression tests of the Empty backend. It is a dummy value. Example: `-DSOCI_EMPTY_TEST_CONNSTR="dummy connection"`

#### IBM DB2

* `SOCI_WITH_DB2` - boolean - OFF - Requests to build [DB2](backends/db2.md) backend.
* `SOCI_DB2_TEST_CONNSTR` - string - :memory: - See [DB2 backend reference](backends/db2.md) for details. Example: `-DSOCI_DB2_TEST_CONNSTR:STRING="DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off"`

#### Firebird

* `SOCI_WITH_FIREBIRD` - boolean - OFF - Requests to build [Firebird](backends/firebird.md) backend
* `SOCI_FIREBIRD_TEST_CONNSTR` - string - :memory: - See [Firebird backend reference](backends/firebird.md) for details. Example: `-DSOCI_FIREBIRD_TEST_CONNSTR:STRING="service=LOCALHOST:/tmp/soci_test.fdb user=SYSDBA password=masterkey"`

#### MySQL

* `SOCI_WITH_MYSQL` - boolean - OFF - Requests to build [MySQL](backends/mysql.md) backend.
* `SOCI_MYSQL_TEST_CONNSTR` - string - :memory: - Connection string to MySQL test database. Format of the string is explained [MySQL backend reference](backends/mysql.md). Example: `-DSOCI_MYSQL_TEST_CONNSTR:STRING="db=mydb user=mloskot password=secret"`

#### ODBC

* `SOCI_WITH_ODBC` - boolean - OFF - Requests to build [ODBC](backends/odbc.md) backend.
* `SOCI_ODBC_TEST_{database}_CONNSTR` - string - :memory: - ODBC Data Source Name (DSN) or ODBC File Data Source Name (FILEDSN) to test database: Microsoft Access (.mdb), Microsoft SQL Server, MySQL, PostgreSQL or any other ODBC SQL data source. {database} is placeholder for name of database driver ACCESS, MYSQL, POSTGRESQL, etc. See [ODBC](backends/odbc.md) backend reference for details. Example: `-DSOCI_ODBC_TEST_POSTGRESQL_CONNSTR="FILEDSN=/home/mloskot/soci/build/test-postgresql.dsn"`

#### Oracle

* `SOCI_WITH_ORACLE` - boolean - OFF - Requests to build [Oracle](backends/oracle.md) backend.
* `SOCI_ORACLE_TEST_CONNSTR` - string - :memory: - Connection string to Oracle test database. Format of the string is explained [Oracle backend reference](backends/oracle.md). Example: `-DSOCI_ORACLE_TEST_CONNSTR:STRING="service=orcl user=scott password=tiger"`


#### PostgreSQL

* `SOCI_WITH_POSTGRESQL` - boolean - OFF - Requests to build [PostgreSQL](backends/postgresql.md) backend.
* `SOCI_POSTGRESQL_TEST_CONNSTR` - string - :memory: - Connection string to PostgreSQL test database. Format of the string is explained PostgreSQL backend reference. Example: `-DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="dbname=mydb user=scott"

#### SQLite 3

* `SOCI_WITH_SQLITE3` - boolean - OFF - Requests to build [SQLite3](backends/sqlite3.md) backend.
* `SOCI_SQLITE3_TEST_CONNSTR` - string - :memory: - Connection string is simply a file path where SQLite3 test database will be created (e.g. /home/john/soci_test.db). Check [SQLite3 backend reference](backends/sqlite3.md) for details. Example: `-DSOCI_SQLITE3_TEST_CONNSTR="my.db"` or `-DSOCI_SQLITE3_TEST_CONNSTR=":memory:"`.

##### Sqlite3 Installation notes (for Windows, use the package manager on Linux instead):
Sqlite3 distributes its source code and libraries seperatly. Also it does only distribute a .dll file for Windows without the import library needed. 
You should create a directory with the structure:
sqlite3
 - include
   sqlite3.h
 - lib
   sqlite3.dll
   sqlite3.def

Then in your lib folder you need to generate a sqlite3.lib file from the .dll and .def.
You need to open your  Visual Studio command prompt, go to the lib directory and use the following command:
```console
lib /DEF:sqlite3.def /OUT:sqlite3.lib /MACHINE:x64
```

Finally add the sqlite3 directory and the sqlite3/lib directory to your PATH Variable.

### Run Tests

To run tests  used you can use the following command:
```console
cmake --build build --target test
```
Make sure BUILD_TESTING is set on otherwise there won't be tests available.

## Using the library

No matter what approach you take, soci has some external dependencies. Mainly the database for each backend you need.
So make sure they are installed correctly (and can be found by cmake e.g. setting the PATH or CMAKE_PREFIX_PATH). You might find installation notes for the backends above if there is anything special you have to take care of.

### find_package (Installation needed)

Using the library is quite straight with cmake forward if you installed soci as described above. Soci provides a SociFonfig.cmake, which makes using it very easy if you. 

In you CmakeLists.txt add something like that: 
```cmake
find_package(Soci COMPONENTS Sqlite3 REQUIRED)

# Main build targets
add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_sqlite3)
```

Have a look at the example folder to see full examples on how to use soci with cmake. 

### fetchContent (No Installation needed)

Using the library with fetchContent is also rather straight forward in general. It's in general easier as you don't have to build the project and install it, but therefore you need slightly more code and configure soci in your project instead.

It's recommend that you put the code for pulling in soci in another scope, so you can set variables like BUILD_SHARED_LIBS without affecting your project or other dependencies. In this example we use the approach to create a dedicated directory dependencies and another one for each dependency you have (and use fetchContent with). This will create a new directory scope and therefore you can set these variables without risk

Warning: Don't do this approach for find_package, targets found by find_package are only available in the same scope / child scope. Therefore it must be used in the top CMakeLists.txt or in a subdirectory where your executable is declared.

dependencies/soci/CMakeLists.txt
```cmake
include(FetchContent)
FetchContent_Declare(Soci
  GIT_REPOSITORY <LinkToGit>
  GIT_TAG        <optionalGitTag/branchName>
)
set(SOCI_WITH_SQLITE3 ON)
set(SOCI_WITH_BOOST ON) # Optional
set(SOCI_WITH_CXX11) # If your compiler supports C++11
FetchContent_MakeAvailable(Soci)
```

dependencies/CMakeLists.txt
```cmake
...
add_subdirectory(soci)
...
```

CMakeLists.txt
```cmake
...
# Dependencies
add_subdirectory(dependencies)

# Main build targets
add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_sqlite3)

...
```

Have a look at the example folder to see full examples on how to use soci with cmake.