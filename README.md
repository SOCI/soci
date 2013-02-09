SOCI - The C++ Database Access Library
======================================

Brief History
-------------
Originally, SOCI was developed by [Maciej Sobczak](http://www.msobczak.com/)
at [CERN](http://www.cern.ch/) as abstraction layer for Oracle,
a **Simple Oracle Call Interface**.
Later, several database backends have been developed for SOCI,
thus the long name has lost its practicality.
Currently, if you like, SOCI may stand for **Simple Open (Database) Call Interface**
or something similar.

> "CERN is also a user of the SOCI library, which serves as a database access
> layer in some of the control system components."

-- Maciej Sobczak at [Inspirel](http://www.inspirel.com/users.html)

Infrastructure
--------------

GitHub hosts SOCI source code repository,
issues tracker and wiki: https://github.com/SOCI

Project website at http://soci.sourceforge.net
Release downloads and mailing lists at 
http://sourceforge.net/projects/soci/

Travis CI service at https://travis-ci.org/SOCI/soci

[![Build Status](https://api.travis-ci.org/SOCI/soci.png)](https://travis-ci.org/SOCI/soci)

License
-------

The SOCI library is distributed under the terms of the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).

Requirements
------------

Core:
* C++ compiler

Backend specific:
* MySQL client library X.Y
* ODBC implementation X.Y
* Oracle OCI library X.Y
* PostgreSQL client library X.Y
* SQLite 3 library X.Y

Git repository of SOCI project
------------------------------

* /src - project source code tree
* /doc - project documentation
* /www - project website
* /build - legacy build configurations (to be removed)
