# SOCI - The C++ Database Access Library

SOCI is a database access library written in C++ that allows embedding
SQL queries in the regular C++ code in the most natural and intuitive way,
while staying entirely within the Standard C++.

## Basic Syntax

Note that all the examples here assume that `using namespaces soci;` directive
is in scope, and use a [previously initialized](connections.md) `sql` object
of type `soci::session`.

The simplest motivating code example for the SQL query that is supposed to
retrieve a single row is:

```cpp
int id = ...;
std::string name;
int salary;

sql << "select name, salary from persons where id = :id",
       use(id), into(name), into(salary);
```

## Basic ORM

The following benefits from extensive support for object-relational mapping:

```cpp
int id = ...;
Person p;

sql << "select first_name, last_name, date_of_birth "
       "from persons where id = :id", use(id), into(p);
```

## Integrations

Integration with STL is also supported:

```cpp
Rowset<std::string> rs = (sql.prepare << "select name from persons");
std::copy(rs.begin(), rs.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
```

SOCI also offers [integration with Boost](boost.md) data types (optional, tuple and fusion) and flexible support for user-defined types.

## Database Backends

SOCI uses the plug-in architecture for backends, allowing it to target various
database servers.

Currently (SOCI 4.1.2), backends for following database systems are supported:

* [DB2](backends/db2.md)
* [Firebird](backends/firebird.md)
* [MySQL](backends/mysql.md)
* [ODBC](backends/odbc.md) (generic backend)
* [Oracle](backends/oracle.md)
* [PostgreSQL](backends/postgresql.md)
* [SQLite3](backends/sqlite3.md)

The intent of the library is to cover as many database technologies as possible.
For this, the project has to rely on volunteer contributions from other programmers,
who have expertise with the existing database interfaces and would like to help
writing dedicated backends.

## Language Bindings

Even though SOCI is mainly a C++ library, it also allows to use it from other programming languages.
Currently the package contains the Ada binding, with more bindings likely to come in the future.
