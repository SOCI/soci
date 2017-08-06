# DB2 Backend Reference

SOCI backend for accessing IBM DB2 database.

## Prerequisites

### Supported Versions

See [Tested Platforms](#tested-platforms).

### Tested Platforms

<table>
<tbody>
<tr><th>DB2 version</th><th>Operating System</th><th>Compiler</th></tr>
<tr><td>-</td><td>Linux PPC64</td><td>GCC</td></tr>
<tr><td>9.1</td><td>Linux</td><td>GCC</td></tr>
<tr><td>9.5</td><td>Linux</td><td>GCC</td></tr>
<tr><td>9.7</td><td>Linux</td><td>GCC</td></tr>
<tr><td>10.1</td><td>Linux</td><td>GCC</td></tr>
<tr><td>10.1</td><td>Windows 8</td><td>Visual Studio 2012</td></tr>
</tbody>
</table>

### Required Client Libraries

The SOCI DB2 backend requires IBM DB2 Call Level Interface (CLI) library.

## Connecting to the Database

On Unix, before using the DB2 backend please make sure, that you have sourced DB2 profile into your environment:

    . ~/db2inst1/sqllib/db2profile

To establish a connection to the DB2 database, create a session object using the <code>DB2</code> backend factory together with the database file name:

    soci::session sql(soci::db2, "your DB2 connection string here");

## SOCI Feature Support

### Dynamic Binding

TODO

### Bulk Operations

Supported, but with caution as it hasn't been extensively tested.

### Transactions

Currently, not supported.

### BLOB Data Type

Currently, not supported.

### Nested Statements

Nesting statements are not processed by SOCI in any special way and they work as implemented by the DB2 database.

### Stored Procedures

Stored procedures are supported, with <code>CALL</code> statement.

## Native API Access

*TODO*

## Backend-specific extensions

None.

## Configuration options

This backend supports `db2_option_driver_complete` option which can be passed to
it via `connection_parameters` class. The value of this option is passed to
`SQLDriverConnect()` function as "driver completion" parameter and so must be
one of `SQL_DRIVER_XXX` values, in the string form. The default value of this
option is `SQL_DRIVER_PROMPT` meaning that the driver will query the user for
the user name and/or the password if they are not stored together with the
connection. If this is undesirable for some reason, you can use `SQL_DRIVER_NOPROMPT` value for this option to suppress showing the message box:

    connection_parameters parameters("db2", "DSN=sample");
    parameters.set_option(db2_option_driver_complete, "0" /* SQL_DRIVER_NOPROMPT */);
    session sql(parameters);

Note, `db2_option_driver_complete` controls driver completion specific to the IBM DB2 driver for ODBC and CLI.
