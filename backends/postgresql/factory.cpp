//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef SOCI_PGSQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_PGSQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


// concrete factory for Empty concrete strategies
postgresql_session_backend * postgresql_backend_factory::make_session(
     std::string const & connectString) const
{
     return new postgresql_session_backend(connectString);
}

postgresql_backend_factory const soci::postgresql;

extern "C"
{

// for dynamic backend loading
SOCI_POSTGRESQL_DECL backend_factory const * factory_postgresql()
{
    return &soci::postgresql;
}

} // extern "C"
