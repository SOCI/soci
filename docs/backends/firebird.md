# Firebird Backend Reference

SOCI backend for accessing Firebird database.

## Prerequisites

### Supported Versions

The SOCI Firebird backend supports versions of Firebird from 1.5 to 2.5 and can be used with
either the client-server or embedded Firebird libraries.
The former is the default, to select the latter set `SOCI_FIREBIRD_EMBEDDED` CMake option to `ON`
value when building.

### Tested Platforms

|Firebird |OS|Compiler|
|--- |--- |--- |
|1.5.2.4731|SunOS 5.10|g++ 3.4.3|
|1.5.2.4731|Windows XP|Visual C++ 8.0|
|1.5.3.4870|Windows XP|Visual C++ 8.0 Professional|
|2.5.2.26540|Debian GNU/Linux 7|g++ 4.7.2|
|2.5.8.27089|macOS High Sierra 10.13.5|AppleClang 9.1.0.9020039|

### Required Client Libraries

The Firebird backend requires Firebird's `libfbclient` client library.
For example, on Ubuntu Linux, for example, `firebird-dev` package and its dependencies are required.

## Connecting to the Database

To establish a connection to a Firebird database, create a Session object using the firebird
backend factory together with a connection string:

```cpp
BackEndFactory const &backEnd = firebird;
session sql(backEnd, "service=/usr/local/firbird/db/test.fdb user=SYSDBA password=masterkey");
```

or simply:

```cpp
session sql(firebird, "service=/usr/local/firbird/db/test.fdb user=SYSDBA password=masterkey");
```

The set of parameters used in the connection string for Firebird is:

* service
* user
* password
* role
* charset

The following parameters have to be provided as part of the connection string : *service*, *user*,
*password*. Role and charset parameters are optional.

Once you have created a `session` object as shown above, you can use it to access the database, for example:

```cpp
int count;
sql << "select count(*) from user_tables", into(count);
```

(See the [connection](../connections.md) and [data binding](../binding.md) documentation
for general information on using the `session` class.)

## SOCI Feature Support

### Dynamic Binding

The Firebird backend supports the use of the SOCI `row` class, which facilitates retrieval of data whose
type is not known at compile time.

When calling `row::get<T>()`, the type you should pass as T depends upon the underlying database type.
For the Firebird backend, this type mapping is:

| Firebird Data Type                      | SOCI Data Type (`data_type`)            | `row::get<T>` specializations     |
| --------------------------------------- | --------------------------------------- | --------------------------------- |
| numeric, decimal (where scale > 0)      | dt_double                               | double                            |
| numeric, decimal [^1] (where scale = 0) | dt_integer, dt_double                   | int, double                       |
| double precision, float                 | dt_double                               | double                            |
| smallint, integer                       | dt_integer                              | int                               |
| char, varchar                           | dt_string                               | std::string                       |
| date, time, timestamp                   | dt_date                                 | std::tm                           |

| Firebird Data Type                      | SOCI Data Type (`db_type`)              | `row::get<T>` specializations     |
| --------------------------------------- | --------------------------------------- | --------------------------------- |
| numeric, decimal (where scale > 0)      | db_double                               | double                            |
| numeric, decimal [^1] (where scale = 0) | db_int16/db_int32/db_int64, db_double   | int16_t/int32_t/int64_t, double   |
| double precision, float                 | db_double                               | double                            |
| smallint                                | db_int16                                | int16_t                           |
| integer                                 | db_int32                                | int32_t                           |
| bigint                                  | db_int64                                | int64_t                           |
| char, varchar                           | db_string                               | std::string                       |
| date, time, timestamp                   | db_date                                 | std::tm                           |

[^1] There is also 64bit integer type for larger values which is
currently not supported.

(See the [dynamic resultset binding](../types.md#dynamic-binding) documentation for general information
on using the `Row` class.)

### Binding by Name

In addition to [binding by position](../binding.md#binding-by-position), the Firebird backend supports
[binding by name](../binding.md#binding-by-name), via an overload of the `use()` function:

```cpp
int id = 7;
sql << "select name from person where id = :id", use(id, "id")
```

It should be noted that parameter binding by name is supported only by means of emulation,
since the underlying API used by the backend doesn't provide this feature.

### Bulk Operations

The Firebird backend has full support for SOCI [bulk operations](../binding.md#bulk-operations) interface.
This feature is also supported by emulation.

### Transactions

[Transactions](../transactions.md) are also fully supported by the Firebird backend.
In fact, an implicit transaction is always started when using this backend if one hadn't been
started by explicitly calling `begin()` before. The current transaction is automatically
committed in `session` destructor.

### BLOB Data Type

The Firebird backend supports working with data stored in columns of type Blob,
via SOCI [BLOB](../lobs.md) class.

It should by noted, that entire Blob data is fetched from database to allow random read and write access.
This is because Firebird itself allows only writing to a new Blob or reading from existing one -
modifications of existing Blob means creating a new one.
Firebird backend hides those details from user.

### RowID Data Type

This feature is not supported by Firebird backend.

### Nested Statements

This feature is not supported by Firebird backend.

### Stored Procedures

Firebird stored procedures can be executed by using SOCI [Procedure](../procedures.md) class.

## Native API Access

SOCI provides access to underlying datbabase APIs via several `get_backend()` functions,
as described in the [Beyond SOCI](../beyond.md) documentation.

The Firebird backend provides the following concrete classes for navite API access:

|Accessor Function|Concrete Class|
|--- |--- |
|session_backend * session::get_backend()|firebird_session_backend|
|statement_backend * statement::get_backend()|firebird_statement_backend|
|blob_backend * blob::get_backend()|firebird_blob_backend|
|rowid_backend * rowid::get_backend()|

## Backend-specific extensions

### firebird_soci_error

The Firebird backend can throw instances of class `firebird_soci_error`, which is publicly derived
from `soci_error` and has an additional public `status_` member containing the Firebird status vector.
