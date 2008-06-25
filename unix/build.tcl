#!/usr/bin/tclsh

# some common compilation settings if you need to change them:

set CXXFLAGS "-Wall -pedantic -Wno-long-long -O2"
set CXXTESTFLAGS "-O2"

if {$tcl_platform(os) == "Darwin"} {
    # special case for Mac OS X
    set SHARED "-dynamiclib -flat_namespace -undefined suppress"
} else {
    set SHARED "-shared"
}


proc printUsageAndExit {} {
    puts "Usage:"
    puts "$ ./build.tcl list-of-targets"
    puts ""
    puts "list of targets can contain any of:"
    puts "core          - the core part of the library"
    puts "oracle        - the static Oracle backend"
    puts "oracle-so     - the shared Oracle backend"
    puts "                Note: before building Oracle backend"
    puts "                set the ORACLE_HOME variable properly."
    puts "postgresql    - the static PostgreSQL backend"
    puts "postgresql-so - the shared PostgreSQL backend"
    puts "mysql         - the static MySQL backend"
    puts "mysql-so      - the shared MySQL backend"
    puts ""
    puts "oracle-test     - the test for Oracle"
    puts "postgresql-test - the test for PostgreSQL"
    puts "mysql-test      - the test for MySQL"
    puts "                  Note: build static core and backends first."
    puts ""
    puts "Example:"
    puts "$ ./build.tcl core mysql"
    puts ""
    puts "After successful build the results are in include, lib and test directories."
    puts "Move/copy the contents of these directories wherever you want."
    exit
}

if {$argc == 0 || $argv == "--help"} {
    printUsageAndExit
}

proc execute {command} {
    puts $command
    eval exec $command
}

proc findBoost {} {
    # candidate directories for local Boost:
    set includeDirs {
        "/usr/local/include"
        "/usr/include"
    }
    set libDirs {
        "/usr/local/lib"
        "/usr/lib"
    }

    set includeDir ""
    foreach I $includeDirs {
        set header "${I}/boost/version.hpp"
        if {[file exists $header]} {
            set includeDir $I
            break
        }
    }
    if {$includeDir == ""} {
        return {}
    }

    set libDir ""
    foreach L $libDirs {
        set library "${L}/libboost_date_time.a"
        if {[file exists $library]} {
            set libDir $L
            break
        }
    }
    if {$libDir == ""} {
        return {}
    }

    return [list $includeDir $libDir]
}

proc buildCore {} {
    global CXXFLAGS

    puts "building static core"

    set cwd [pwd]
    cd "../../src/core"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS"
    }

    execute "ar rv libsoci_core.a [glob *.o]"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/core/libsoci_core.a lib"
    eval exec mkdir -p "include"
    execute "cp [glob ../../src/core/*.h] include"
}

proc buildCoreSo {} {
    global CXXFLAGS SHARED

    puts "building shared core"

    set cwd [pwd]
    cd "../../src/core"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS -fPIC"
    }

    execute "g++ $SHARED -o libsoci_core.so [glob *.o]"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/core/libsoci_core.so lib"
    eval exec mkdir -p "include"
    execute "cp [glob ../../src/core/*.h] include"
}

proc findPostgreSQL {} {
    # candidate directories for local PostgreSQL:
    set includeDirs {
        "/usr/local/pgsql/include"
        "/usr/local/include/pgsql"
        "/usr/include/pgsql"
        "/usr/include"
    }
    set libDirs {
        "/usr/local/pgsql/lib"
        "/usr/local/lib/pgsql"
        "/usr/lib/pgsql"
        "/usr/lib"
    }

    set includeDir ""
    foreach I $includeDirs {
        set header "${I}/libpq/libpq-fs.h"
        if {[file exists $header]} {
            set includeDir $I
            break
        }
    }
    if {$includeDir == ""} {
        return {}
    }

    set libDir ""
    foreach L $libDirs {
        set library "${L}/libpq.a"
        if {[file exists $library]} {
            set libDir $L
            break
        }
    }
    if {$libDir == ""} {
        return {}
    }

    return [list $includeDir $libDir]
}

proc buildPostgreSQL {} {
    global CXXFLAGS

    puts "building static PostgreSQL"

    set dirs [findPostgreSQL]
    if {$dirs == {}} {
        puts "cannot find PostgreSQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/postgresql"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS -I../../core -I${includeDir}"
    }

    execute "ar rv libsoci_postgresql.a [glob *.o]"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/backends/postgresql/libsoci_postgresql.a lib"
    eval exec mkdir -p "include"
    execute "cp ../../src/backends/postgresql/soci-postgresql.h include"
}

proc buildPostgreSQLSo {} {
    global CXXFLAGS SHARED

    puts "building shared PostgreSQL"

    set dirs [findPostgreSQL]
    if {$dirs == {}} {
        puts "cannot find PostgreSQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/postgresql"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS -fPIC -I../../core -I${includeDir}"
    }

    execute "g++ $SHARED -o libsoci_postgresql.so [glob *.o] -L${libDir} -lpq"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/backends/postgresql/libsoci_postgresql.so lib"
    eval exec mkdir -p "include"
    execute "cp ../../src/backends/postgresql/soci-postgresql.h include"
}

proc buildPostgreSQLTest {} {
    global CXXTESTFLAGS

    puts "building PostgreSQL test"

    set dirs [findPostgreSQL]
    if {$dirs == {}} {
        puts "cannot find PostgreSQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set dirs [findBoost]
    if {$dirs == {}} {
        puts "cannot find Boost library files, skipping this target"
        return
    }

    set boostIncludeDir [lindex $dirs 0]
    set boostLibDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/postgresql/test"
    execute "g++ test-postgresql.cpp -o test-postgresql $CXXTESTFLAGS -I.. -I../../../core -I../../../core/test -I${includeDir} -I${boostIncludeDir} -L../../../../build/unix/lib -L${libDir} -L${boostLibDir} -lsoci_core -lsoci_postgresql -lboost_date_time -ldl -lpq"
    cd $cwd
    eval exec mkdir -p "tests"
    execute "cp ../../src/backends/postgresql/test/test-postgresql tests"
}

proc findMySQL {} {
    # candidate directories for local MySQL:
    set includeDirs {
        "/usr/local/include/mysql"
        "/usr/include/mysql"
        "/usr/include"
    }
    set libDirs {
        "/usr/local/lib/mysql"
        "/usr/lib/mysql"
        "/usr/lib"
    }

    set includeDir ""
    foreach I $includeDirs {
        set header "${I}/mysql.h"
        if {[file exists $header]} {
            set includeDir $I
            break
        }
    }
    if {$includeDir == ""} {
        return {}
    }

    set libDir ""
    foreach L $libDirs {
        set library "${L}/libmysqlclient.a"
        if {[file exists $library]} {
            set libDir $L
            break
        }
    }
    if {$libDir == ""} {
        return {}
    }

    return [list $includeDir $libDir]
}

proc buildMySQL {} {
    global CXXFLAGS

    puts "building static MySQL"

    set dirs [findMySQL]
    if {$dirs == {}} {
        puts "cannot find MySQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/mysql"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS -I../../core -I${includeDir}"
    }

    execute "ar rv libsoci_mysql.a [glob *.o]"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/backends/mysql/libsoci_mysql.a lib"
    eval exec mkdir -p "include"
    execute "cp ../../src/backends/mysql/soci-mysql.h include"
}

proc buildMySQLSo {} {
    global CXXFLAGS SHARED

    puts "building shared MySQL"

    set dirs [findMySQL]
    if {$dirs == {}} {
        puts "cannot find MySQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/mysql"
    foreach cppFile [glob "*.cpp"] {
        execute "g++ -c $cppFile $CXXFLAGS -fPIC -I../../core -I${includeDir}"
    }

    execute "g++ $SHARED -o libsoci_mysql.so [glob *.o] -L${libDir} -lmysqlclient -lz"
    cd $cwd
    eval exec mkdir -p "lib"
    execute "cp ../../src/backends/mysql/libsoci_mysql.so lib"
    eval exec mkdir -p "include"
    execute "cp ../../src/backends/mysql/soci-mysql.h include"
}

proc buildMySQLTest {} {
    global CXXTESTFLAGS

    puts "building MySQL test"

    set dirs [findMySQL]
    if {$dirs == {}} {
        puts "cannot find MySQL library files, skipping this target"
        return
    }

    set includeDir [lindex $dirs 0]
    set libDir [lindex $dirs 1]

    set dirs [findBoost]
    if {$dirs == {}} {
        puts "cannot find Boost library files, skipping this target"
        return
    }

    set boostIncludeDir [lindex $dirs 0]
    set boostLibDir [lindex $dirs 1]

    set cwd [pwd]
    cd "../../src/backends/mysql/test"
    execute "g++ test-mysql.cpp -o test-mysql $CXXTESTFLAGS -I.. -I../../../core -I../../../core/test -I${includeDir} -I${boostIncludeDir} -L../../../../build/unix/lib -L${libDir} -L${boostLibDir} -lsoci_core -lsoci_mysql -lboost_date_time -ldl -lmysqlclient -lz"
    cd $cwd
    eval exec mkdir -p "tests"
    execute "cp ../../src/backends/postgresql/test/test-postgresql tests"
}

foreach target $argv {
    switch -exact $target {
        core buildCore
        core-so buildCoreSo
        oracle buildOracle
        oracle-so buildOracleSo
        postgresql buildPostgreSQL
        postgresql-so buildPostgreSQLSo
        postgresql-test buildPostgreSQLTest
        mysql buildMySQL
        mysql-so buildMySQLSo
        mysql-test buildMySQLTest
        default {
            puts "unknown target $target - skipping"
        }
    }
}
