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
cmake -S . - build  -DWITH_SQLITE3=ON -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF
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

* `SOCI_CXX11` - boolean - Request to compile in C++11 compatibility mode. Default is `OFF`, unless [CMAKE_CXX_STANDARD](https://cmake.org/cmake/help/v3.1/variable/CMAKE_CXX_STANDARD.html) with version `11` or later is given in the command line.
* `BUILD_SHARED_LIBS` - boolean - Request to build shared libraries for SOCI core and all successfully configured backends. Default is `OFF`.
* `BUILD_TESTING` - boolean - Request to build regression tests for SOCI core and all successfully configured backends.

#### Empty (sample backend)

* `SOCI_EMPTY` - boolean - Builds the [sample backend](backends/index.md) called Empty. Always ON by default.
* `SOCI_EMPTY_TEST_CONNSTR` - string - Connection string used to run regression tests of the Empty backend. It is a dummy value. Example: `-DSOCI_EMPTY_TEST_CONNSTR="dummy connection"`

#### IBM DB2

* `WITH_DB2` - boolean - Requests to build [DB2](backends/db2.md) backend.

#### Firebird

* `WITH_FIREBIRD` - boolean - Requests to build [Firebird](backends/firebird.md) backend

#### MySQL

* `WITH_MYSQL` - boolean - Requests to build [MySQL](backends/mysql.md) backend.

#### ODBC

* `WITH_ODBC` - boolean - Requests to build [ODBC](backends/odbc.md) backend.

#### Oracle

* `WITH_ORACLE` - boolean - Requests to build [Oracle](backends/oracle.md) backend.

#### PostgreSQL

* `WITH_POSTGRESQL` - boolean - Requests to build [PostgreSQL](backends/postgresql.md) backend.

#### SQLite 3

* `WITH_SQLITE3` - boolean - Requests to build [SQLite3](backends/sqlite3.md) backend.

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

## Using the library

Using the library is quite straight with cmake forward if you installed soci as described above. Soci provides a SociFonfig.cmake, which makes using it very easy if you. 

In you CmakeLists.txt add something like that: 
```cmake
find_package(SOCI COMPONENTS sqlite3 REQUIRED)

# Main build targets
add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_sqlite3)
```

Have a look at the example folder to see full examples on how to use soci with cmake. 

You might be also able to use a different method like fetchContent, it isn't tested yet though.
