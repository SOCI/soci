# SQLite3 Backend Reference

SOCI backend for accessing SQLite 3 database.

## Prerequisites

### Supported Versions

The SOCI SQLite3 backend is supported for use with SQLite3 >= 3.1

### Tested Platforms

|SQLite3|OS|Compiler|
|--- |--- |--- |
|3.12.1|Windows Server 2016|MSVC++ 14.1|
|3.12.1|Windows Server 2012 R2|MSVC++ 14.0|
|3.12.1|Windows Server 2012 R2|MSVC++ 12.0|
|3.12.1|Windows Server 2012 R2|MSVC++ 11.0|
|3.12.1|Windows Server 2012 R2|Mingw-w64/GCC 4.8|
|3.7.9|Ubuntu 12.04|g++ 4.6.3|
|3.4.0|Windows XP|(cygwin) g++ 3.4.4|
|3.4.0|Windows XP|Visual C++ 2005 Express Edition|
|3.3.8|Windows XP|Visual C++ 2005 Professional|
|3.5.2|Mac OS X 10.5|g++ 4.0.1|
|3.3.4|Ubuntu 5.1|g++ 4.0.2|
|3.3.4|Windows XP|(cygwin) g++ 3.3.4|
|3.3.4|Windows XP|Visual C++ 2005 Express Edition|
|3.2.1|Linux i686 2.6.10-gentoo-r6|g++ 3.4.5|
|3.1.3|Mac OS X 10.4|g++ 4.0.1|
|3.24.0|macOS High Sierra 10.13.5|AppleClang 9.1.0.9020039|

### Required Client Libraries

The SOCI SQLite3 backend requires SQLite3's `libsqlite3` client library.

### Connecting to the Database

To establish a connection to the SQLite3 database, create a Session object using the `SQLite3` backend factory together with the database file name:

```cpp
session sql(sqlite3, "database_filename");

// or:

session sql("sqlite3", "db=db.sqlite timeout=2 shared_cache=true");
```

The set of parameters used in the connection string for SQLite is:

* `dbname` or `db` - this parameter is required unless the entire connection
string is just the database name, in which case it must not contain any `=`
signs.
* `timeout` - set sqlite busy timeout (in seconds) ([link](https://www.sqlite.org/c3ref/busy_timeout.html))
* `readonly` - open database in read-only mode instead of the default read-write (note that the database file must already exist in this case, see [the documentation](https://www.sqlite.org/c3ref/open.html))
* `nocreate` - open an existing database without creating a new one if it doesn't already exist (by default, a new database file is created).
* `synchronous` - set the pragma synchronous flag ([link](https://www.sqlite.org/pragma.html#pragma_synchronous))
* `shared_cache` - enable or disabled shared pager cache ([link](https://www.sqlite.org/c3ref/enable_shared_cache.html))
* `vfs` - set the SQLite VFS used to as OS interface. The VFS should be registered before opening the connection, see [the documenation](https://www.sqlite.org/vfs.html)
* `foreign_keys` - set the pragma `foreign_keys` flag ([link](https://www.sqlite.org/pragma.html#pragma_foreign_keys)).

Boolean options `readonly`, `nocreate`, and `shared_cache` can be either
specified without any value, which is equivalent to setting them to `1`, or set
to one of `1`, `yes`, `true` or `on` to enable the option or `0`, `no`, `false`
or `off` to disable it. Specifying any other value results in an error.

Once you have created a `session` object as shown above, you can use it to access the database, for example:

```cpp
int count;
sql << "select count(*) from invoices", into(count);
```

(See the [connection](../connections.md) and [data binding](../binding.md) documentation for general information on using the `session` class.)

## SOCI Feature Support

### Dynamic Binding

The SQLite3 backend supports the use of the SOCI `row` class, which facilitates retrieval of data whose type is not known at compile time.

When calling `row::get<T>()`, the type you should pass as T depends upon the underlying database type.

For the SQLite3 backend, this type mapping is complicated by the fact the SQLite3 does not enforce [types][INTEGER_PRIMARY_KEY] and makes no attempt to validate the type names used in table creation or alteration statements. SQLite3 will return the type as a string, SOCI will recognize the following strings and match them the corresponding SOCI types:

| SQLite3 Data Type                                                                                            | SOCI Data Type (`data_type`) | `row::get<T>` specializations |
| ------------------------------------------------------------------------------------------------------------ | ---------------------------- | ----------------------------- |
| *float*, *decimal*, *double*, *double precision*, *number*, *numeric*, *real*                                | dt_double                    | double                        |
| *int*, *integer*, *int2*, *mediumint*, *boolean*                                                             | dt_integer                   | int                           |
| *int8*, *bigint*                                                                                             | dt_long_long                 | long long                     |
| *unsigned big int*                                                                                           | dt_unsigned_long_long        | unsigned long long            |
| *text*, *char*, *character*, *clob*, *native character*, *nchar*, *nvarchar*, *varchar*, *varying character* | dt_string                    | std::string                   |
| *date*, *time*, *datetime*                                                                                   | dt_date                      | std::tm                       |

| SQLite3 Data Type                                                                                            | SOCI Data Type (`db_type`)   | `row::get<T>` specializations |
| ------------------------------------------------------------------------------------------------------------ | ---------------------------- | ----------------------------- |
| *float*, *decimal*, *double*, *double precision*, *number*, *numeric*, *real*                                | db_double                    | double                        |
| *tinyint*                                                                                                    | db_int8                      | int8_t                        |
| *smallint*                                                                                                   | db_int16                     | int16_t                       |
| *int*, *integer*, *int2*, *mediumint*, *boolean*                                                             | db_int32                     | int32_t                       |
| *int8*, *bigint*                                                                                             | db_int64                     | int64_t                       |
| *unsigned big int*                                                                                           | db_uint64                    | uint64_t                      |
| *text*, *char*, *character*, *clob*, *native character*, *nchar*, *nvarchar*, *varchar*, *varying character* | db_string                    | std::string                   |
| *date*, *time*, *datetime*                                                                                   | db_date                      | std::tm                       |

Another consequence of SQLite's lack of type checking is that it is possible to store arbitrarily large integers in a column, even if the type it has
been created with wouldn't allow for that. Thus, it is possible that a column of type `integer` (which should correspond to the possible values of a
32-bit signed integer type) contains a value that overflows a 32-bit integer. In such cases, trying to query the data via `row::get<T>`, with `T` being
the type as given in the table above, will throw an exception as the selected value would overflow the provided type. You may instead use a `T` that
can holder a wider range (e.g. a 64-bit integer) in order to avoid this exception.

[INTEGER_PRIMARY_KEY] : There is one case where SQLite3 enforces type. If a column is declared as "integer primary key", then SQLite3 uses that as an alias to the internal ROWID column that exists for every table.  Only integers are allowed in this column.

(See the [dynamic resultset binding](../types.md#dynamic-binding) documentation for general information on using the `row` class.)

### Binding by Name

In addition to [binding by position](../binding.md#binding-by-position), the SQLite3 backend supports [binding by name](../binding.md#binding-by-name), via an overload of the `use()` function:

```cpp
int id = 7;
sql << "select name from person where id = :id", use(id, "id")
```

The backend also supports the SQLite3 native numbered syntax, "one or more literals can be replace by a parameter "?" or ":AAA" or "@AAA" or "$VVV" where AAA is an alphanumeric identifier and VVV is a variable name according to the syntax rules of the TCL programming language." [[1]](https://www.sqlite.org/capi3ref.html#sqlite3_bind_int):

```cpp
int i = 7;
int j = 8;
sql << "insert into t(x, y) values(?, ?)", use(i), use(j);
```

### Bulk Operations

The SQLite3 backend has full support for SOCI's [bulk operations](../binding.md#bulk-operations) interface.  However, this support is emulated and is not native.

### Transactions

[Transactions](../transactions.md) are also fully supported by the SQLite3 backend.

### BLOB Data Type

The SQLite3 backend supports working with data stored in columns of type Blob, via SOCI's [BLOB](../lobs.md) class. Because of SQLite3 general typelessness the column does not have to be declared any particular type.

### RowID Data Type

In SQLite3 RowID is an integer. "Each entry in an SQLite table has a unique integer key called the "rowid". The rowid is always available as an undeclared column named ROWID, OID, or _ROWID_. If the table has a column of type INTEGER PRIMARY KEY then that column is another an alias for the rowid."[[2]](https://www.sqlite.org/capi3ref.html#sqlite3_last_insert_rowid)

### Nested Statements

Nested statements are not supported by SQLite3 backend.

### Stored Procedures

Stored procedures are not supported by SQLite3 backend

## Native API Access

SOCI provides access to underlying datbabase APIs via several `get_backend()` functions, as described in the [beyond SOCI](../beyond.md) documentation.

The SQLite3 backend provides the following concrete classes for navite API access:

|Accessor Function|Concrete Class|
|--- |--- |
|session_backend* session::get_backend()|sqlie3_session_backend|
|statement_backend* statement::get_backend()|sqlite3_statement_backend|
|rowid_backend* rowid::get_backend()|sqlite3_rowid_backend|

## Backend-specific extensions

### SQLite3 result code support

SQLite3 result code is provided via the backend specific `sqlite3_soci_error` class. Catching the backend specific error yields the value of SQLite3 result code via the `result()` method.

## Configuration options

None
