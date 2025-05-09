# Errors

All DB-related errors manifest themselves as exceptions of type `soci_error`, which is derived from `std::runtime_error`.
This allows to handle database errors within the standard exception framework:

```cpp
int main()
{
    try
    {
        // regular code
    }
    catch (std::exception const & e)
    {
        cerr << "Bang! " << e.what() << endl;
    }
}
```

The `soci_error` class exposes the following public functions:

The `get_error_message() const` function returns `std::string` with a brief error message, without any additional information that can be present in the full error message returned by `what()`.

The `get_error_category() const` function returns one of the `error_category` enumeration values, which allows the user to portably react to some subset of common errors.
For example, `connection_error` or `constraint_violation` have meanings that are common across different database backends, even though the actual mechanics might differ.
Please note that error categories are not universally supported and there is no claim that all possible errors that are reported by the database server are covered or interpreted. If the error category is not recognized by the backend, it defaults to `unknown`.

## Backend-specific errors

Some errors originate in SOCI itself, while others are thrown by the database-specific backend and may contain extra information about the error provided by the database itself. `soci_error` provides `get_backend_name()` function which returns a non-empty string with the name of the backend that threw the error to distinguish such errors from the errors generated by SOCI itself, for which this function returns an empty string.

For the backend-specific errors, either `get_backend_error_code()`, returning backend-specific integer error code, or `get_sqlstate()`, returning a 5 character string describing the error in more details, can be used to identify the error more precisely. `get_backend_error_code()` returns 0 if integer error codes are not supported by the backend and `get_sqlstate()` returns an empty string if the backend doesn't provide `SQLSTATE`, please see the backend-specific sections below for more details.

These functions can be used without including the backend-specific headers, but this means that the backend name string must be checked manually, instead of relying on catching the correct exception class. Here is an example of doing it:

```cpp
#include <soci/soci.h>

#include <iostream>

using namespace std;

int main()
{
    try
    {
        // code using SOCI
    }
    catch (soci::exception const & e)
    {
        std::string const backend = e.get_backend_name();
        if ( backend == "postgresql" )
        {
            if ( e.get_sqlstate() == "54001" )
            {
                // Handle PostgreSQL "Statement too complex" error.
            }
        }
        else if ( backend == "sqlite3" )
        {
            if ( e.get_backend_error_code() == 18 /* SQLITE_TOOBIG */ )
            {
                // Handle SQLite "Statement too long" error.
            }
        }
        else
        {
            cerr << "Some other error: " << e.what() << endl;
        }
    }
}
```

## Firebird

The Firebird backend can throw instances of the `firebird_soci_error`, which is publicly derived from `soci_error` and overrides `get_backend_error_code()` to return the first Firebird status code, if available, or 0 otherwise. This exception class also has a public member field `status_` which is a vector containing all status codes associated with the error:

```cpp
#include <soci/soci.h>
#include <soci/firebird/soci-firebird.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::firebird_soci_error const & e)
    {
        cerr << "Firebird error: " << e.get_backend_error_code()
            << " " << e.what() << endl;

        // If necessary, e.status_ vector may be examined for additional status codes.
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```

## MySQL

The MySQL backend can throw instances of the `mysql_soci_error`, which is publicly derived from `soci_error` and overrides `get_backend_error_code()` to return the MySQL error code (as returned by `mysql_errno()`):

```cpp
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::mysql_soci_error const & e)
    {
        cerr << "MySQL error: " << e.get_backend_error_code()
            << " " << e.what() << endl;
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```

## ODBC

The ODBC backend can throw instances of the `odbc_soci_error`, which is publicly derived from `soci_error` and overrides both `get_backend_error_code()` and `get_sqlstate()` to return the numeric ODBC error and `SQLSTATE` associated with it, respectively:

```cpp
#include <soci/soci.h>
#include <soci/odbc/soci-odbc.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::odbc_soci_error const & e)
    {
        cerr << "ODBC error: " << e.get_backend_error_code()
            << " " << e.get_sqlstate() << " " << e.what() << endl;
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```

## Oracle

The Oracle backend can throw the instances of the `oracle_soci_error`, which is publicly derived from `soci_error` and overrides `get_backend_error_code()` to return the Oracle error code:

```cpp
#include <soci/soci.h>
#include <soci/oracle/soci-oracle.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::oracle_soci_error const & e)
    {
        cerr << "Oracle error: " << e.get_backend_error_code()
            << " " << e.what() << endl;
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```

## PostgreSQL

The PostgreSQL backend can throw the instances of the `postgresql_soci_error`, which is publicly derived from `soci_error` and overrides `get_sqlstate()` member function to return PostgreSQL native error code, which is a 5 character `SQLSTATE` string.

```cpp
#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::postgresql_soci_error const & e)
    {
        cerr << "PostgreSQL error: " << e.get_sqlstate()
            << " " << e.what() << endl;
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```

## SQLite

The SQLite backend can throw the instances of the `sqlite3_soci_error`, which is publicly derived from `soci_error` and overrides `get_backend_error_code()` to return the (possibly extended) SQLite result code:

```cpp
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

int main()
{
    try
    {
        // regular code
    }
    catch (soci::sqlite3_soci_error const & e)
    {
        cerr << "SQLite error: " << e.get_backend_error_code()
            << " " << e.what() << endl;
    }
    catch (soci::exception const & e)
    {
        cerr << "Some other error: " << e.what() << endl;
    }
}
```
