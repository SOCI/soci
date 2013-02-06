SOCI - The C++ Database Access Library
======================================

Brief History
-------------
Originally, SOCI was developed by Maciej Sobczak in CERN as
abstraction layer for Oracle, a Simple Oracle Call Interface.
Later, serveral database backends have been developed and added to SOCI.
Currently, SOCI may stand for something like Simple Open (Database) Call Interface.

Infrastructure
--------------

GitHub hosts SOCI source code repository,
issues tracker and wiki: https://github.com/SOCI

Project website at http://soci.sourceforge.net
Release downloads and mailing lists at 
http://sourceforge.net/projects/soci/

Travis CI service at https://travis-ci.org/SOCI/soci

[![Build Status](https://api.travis-ci.org/SOCI/soci.png)](https://travis-ci.org/SOCI/soci)

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
