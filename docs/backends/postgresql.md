# PostgreSQL Backend Reference

SOCI backend for accessing PostgreSQL database.

## Prerequisites

### Supported Versions

The SOCI PostgreSQL backend is supported for use with PostgreSQL >= 9.0, although older versions may suffer from limited feature support. See below for details.

### Tested Platforms

|PostgreSQL|OS|Compiler|
|--- |--- |--- |
| 14|macOS 11.7|AppleClang 13|
| 14|Ubuntu 22.04|gcc 11.4|
| 13|Windows Server 2019|MSVS 2022|
| 12|Windows Server 2019|MSVS 2019|
| 11|Windows Server 2016|MSVS 2017|
| 10|Windows Server 2012 R2|MSVS 2015|
|9.4|Windows Server 2012 R2|Mingw-w64/GCC 8.1|

### Required Client Libraries

The SOCI PostgreSQL backend requires PostgreSQL's `libpq` client library.

Note that the SOCI library itself depends also on `libdl`, so the minimum set of libraries needed to compile a basic client program is:

```console
-lsoci_core -lsoci_postgresql -ldl -lpq
```

### Connecting to the Database

To establish a connection to the PostgreSQL database, create a `session` object using the `postgresql` backend factory together with a connection string:

```cpp
session sql(postgresql, "dbname=mydatabase");

// or:
session sql("postgresql", "dbname=mydatabase");

// or:
session sql("postgresql://dbname=mydatabase");
```

The set of parameters used in the connection string for PostgreSQL is the same as accepted by the [PQconnectdb](https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTDB) function from the `libpq` library, however in addition to standard PostgreSQL connection parameters, SOCI PostgreSQL backend accepts a couple of additional ones:

* `tracefile`: if specified, enables tracing all database activity using [PQtrace()](https://www.postgresql.org/docs/current/libpq-control.html#LIBPQ-PQTRACE) into the file with the given path. Note that this file is overwritten by default, prepend it with a plus sign, i.e. use `tracefile=+/path/to/file`, to append to the file instead.
* `singlerow` or `singlerows`: if set to `true` or `yes`, enables single-row mode for the session (see next section).

Another parameter is handled specially: while libpq client library supports `tcp_user_timeout` parameter, it only provides support for it under Linux. SOCI also handles this parameter, which can change the time before a broken connection times out (which depends on the system settings but is typically relatively long), under Windows. It handles it in the same way as libpq itself, i.e. the value of this parameter is expressed in milliseconds, but because Windows sockets only use second granularity, SOCI will round the value to the nearest second. In particular, this means that if this parameter is not 0 (which means to use the system default, i.e. is same as not specifying it at all), then it will be always set to at least 1 second. Also note that negative values for this option are currently ignored, but shouldn't be used as their interpretation may change in the future versions of the library.

Once you have created a `session` object as shown above, you can use it to access the database, for example:

```cpp
int count;
sql << "select count(*) from invoices", into(count);
```

(See the [connection](../connections.md) and [data binding](../binding.md) documentation for general information on using the `session` class.)

#### Single Row Mode

If this mode is enabled, e.g.

```cpp
session sql(postgresql, "dbname=mydatabase singlerows=true");
```

then queries will be executed in the single-row mode, which prevents the client library from loading full query result sets into memory and instead fetches rows one by one, as they are requested by the statement's fetch() function. This mode can be of interest to those users who want to make their client applications more responsive (with more fine-grained operation) by avoiding potentially long blocking times when complete query results are loaded to client's memory.
Note that in the single-row operation:

* bulk queries are not supported, and
* in order to fulfill the expectations of the underlying client library, the complete rowset has to be exhausted before executing further queries on the same session.

## SOCI Feature Support

### Dynamic Binding

The PostgreSQL backend supports the use of the SOCI `row` class, which facilitates retrieval of data whose type is not known at compile time.

When calling `row::get<T>()`, the type you should pass as `T` depends upon the underlying database type. For the PostgreSQL backend, this type mapping is:

| PostgreSQL Data Type                                         | SOCI Data Type (`data_type`) | `row::get<T>` specializations |
| ------------------------------------------------------------ | ---------------------------- | ----------------------------- |
| numeric, real, double                                        | dt_double                    | double                        |
| boolean, smallint, integer                                   | dt_integer                   | int                           |
| int8                                                         | dt_long_long                 | long long                     |
| oid                                                          | dt_integer                   | unsigned long                 |
| char, varchar, text, cstring, bpchar                         | dt_string                    | std::string                   |
| abstime, reltime, date, time, timestamp, timestamptz, timetz | dt_date                      | std::tm                       |

| PostgreSQL Data Type                                         | SOCI Data Type (`db_type`)   | `row::get<T>` specializations |
| ------------------------------------------------------------ | ---------------------------- | ----------------------------- |
| numeric, real, double                                        | db_double                    | double                        |
| boolean                                                      | db_int8                      | int8_t                        |
| smallint                                                     | db_int16                     | int16_t                       |
| integer                                                      | db_int32                     | int32_t                       |
| int8                                                         | db_int64                     | int64_t                       |
| oid                                                          | db_int32                     | int32_t                       |
| char, varchar, text, cstring, bpchar                         | db_string                    | std::string                   |
| abstime, reltime, date, time, timestamp, timestamptz, timetz | db_date                      | std::tm                       |

(See the [dynamic resultset binding](../types.md#dynamic-binding) documentation for general information on using the `row` class.)

### Binding by Name

In addition to [binding by position](../binding.md#binding-by-position), the PostgreSQL backend supports [binding by name](../binding.md#binding-by-name), via an overload of the `use()` function:

```cpp
int id = 7;
sql << "select name from person where id = :id", use(id, "id")
```

### Bulk Operations

The PostgreSQL backend has full support for SOCI's [bulk operations](../binding.md#bulk-operations) interface.

### Transactions

[Transactions](../transactions.md) are also fully supported by the PostgreSQL backend.

### blob Data Type

The PostgreSQL backend supports working with data stored in columns of type Blob, via SOCI's [blob](../lobs.md) class.

Note that 64-bit offsets require PostgreSQL client library 9.3 or later.

### rowid Data Type

The concept of row identifier (OID in PostgreSQL) is supported via SOCI's [rowid](../api/client.md#class-rowid) class.

### Nested Statements

Nested statements are not supported by PostgreSQL backend.

### Stored Procedures

PostgreSQL stored procedures can be executed by using SOCI's [procedure](../procedures.md) class.

## Native API Access

SOCI provides access to underlying database APIs via several `get_backend()` functions, as described in the [beyond SOCI](../beyond.md) documentation.

The PostgreSQL backend provides the following concrete classes for native API access:

|Accessor Function|Concrete Class|
|--- |--- |
|session_backend * session::get_backend()|postgresql_session_backend|
|statement_backend * statement::get_backend()|postgresql_statement_backend|
|blob_backend * blob::get_backend()|postgresql_blob_backend|
|rowid_backend * rowid::get_backend()|postgresql_rowid_backend|

## Backend-specific extensions

### uuid Data Type

The PostgreSQL backend supports working with data stored in columns of type UUID via simple string operations. All string representations of UUID supported by PostgreSQL are accepted on input, the backend will return the standard
format of UUID on output. See the test `test_uuid_column_type_support` for usage examples.

### Optimizing Prepared Statements Deallocation

By default, this backend executes `DEALLOCATE` statement for each prepared statement when it is destroyed. This is correct, but may be surprisingly time-consuming and is not necessary for short-running programs as all resources associated with a connection, including any statements prepared by it, are deallocated when it is closed in any case.

To save time spent on deallocation, `postgresql_session_backend::set_deallocate_prepared_statements(bool)` function can be used to disable automatic deallocation of prepared statements. This function can be called at any time, but will only affect statements destroyed after its call, so it is recommended to do it early.

Not deallocating prepared statements may lead to memory leaks in long-running programs, so in case of such programs it is recommended to only call `set_deallocate_prepared_statements(false)` before starting a performance-sensitive operation involving many prepared statements, and then call `deallocate_all_prepared_statements()` once the operation is finished and no prepared statements remain, followed by a call to `set_deallocate_prepared_statements(true)` to restore the default behaviour.
