# SOCI - The C++ Database Access Library

[![GitHub release](https://img.shields.io/github/tag/SOCI/soci.svg)](https://github.com/SOCI/soci/releases/tag/v4.0.3)
[![GitHub commits](https://img.shields.io/github/commits-since/SOCI/soci/v4.0.3.svg)](https://github.com/SOCI/soci/tree/release/4.0)

[![Website](https://img.shields.io/website-up-down-green-red/http/shields.io.svg?label=soci.sourceforge.net)](http://soci.sourceforge.net)
[![SourceForge](https://img.shields.io/sourceforge/dm/soci.svg)](https://sourceforge.net/projects/soci/files/)

[![Gitter](https://img.shields.io/gitter/room/SOCI/soci.svg)](https://gitter.im/SOCI/soci)
[![Mailing Lists](https://img.shields.io/badge/mailing--lists-ok-yellowgreen.svg)](https://sourceforge.net/p/soci/mailman/)
[![StackExchange](https://img.shields.io/stackexchange/stackoverflow/t/soci.svg)](https://stackoverflow.com/questions/tagged/soci)

## Build Status

| Branches    | GitHub Actions | AppVeyor-CI | Documentation | Coverity Scan  |
|-------------|----------------|-------------|---------------|----------------|
| master      | [![Build Status](https://github.com/SOCI/soci/actions/workflows/ci.yml/badge.svg)](https://github.com/SOCI/soci/actions) | [![Build status](https://ci.appveyor.com/api/projects/status/dtp5mvbeyu9aqupr/branch/master?svg=true)](https://ci.appveyor.com/project/SOCI/soci/branch/master) | [![Docs Status](https://circleci.com/gh/SOCI/soci/tree/master.svg?style=svg&circle-token=5d31c692ed5fcffa5c5fc6b7fe2257b34d78f3c9)](https://circleci.com/gh/SOCI/soci/tree/master) | [![Coverage](https://scan.coverity.com/projects/6581/badge.svg)](https://scan.coverity.com/projects/soci-soci) |
| release/4.0 | [![Build Status](https://github.com/SOCI/soci/actions/workflows/ci.yml/badge.svg?branch=release/4.0)](https://github.com/SOCI/soci/actions) | [![Build status](https://ci.appveyor.com/api/projects/status/dtp5mvbeyu9aqupr/branch/release/4.0?svg=true)](https://ci.appveyor.com/project/SOCI/soci/branch/release/4.0) | [![Docs Status](https://circleci.com/gh/SOCI/soci/tree/release%2F4.0.svg?style=svg&circle-token=5d31c692ed5fcffa5c5fc6b7fe2257b34d78f3c9)](https://circleci.com/gh/SOCI/soci/tree/release%2F4.0) | |

## History

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

## License

SOCI library is distributed under the terms of the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).

## Requirements

Core:

* Compile with C++14 support (please use 4.0 series for C++98 support) or MSVS 2015 or later.
* Optionally: Boost C++ Libraries (header-only and Boost.DateTime).

Backend specific client libraries for:

* DB2
* Firebird
* MySQL
* ODBC with specific database driver
* Oracle
* PostgreSQL
* SQLite 3

See documentation at [soci.sourceforge.net](http://soci.sourceforge.net) for details

[BSL](http://www.boost.org/LICENSE_1_0.txt) &copy;
[Maciej Sobczak](http://github.com/msobczak) and [contributors](https://github.com/SOCI/soci/graphs/contributors).
