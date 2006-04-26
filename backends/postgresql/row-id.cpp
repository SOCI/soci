//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-postgresql.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>
#include <libpq/libpq-fs.h>


#ifdef SOCI_PGSQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_PGSQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


PostgreSQLRowIDBackEnd::PostgreSQLRowIDBackEnd(
    PostgreSQLSessionBackEnd &session)
{
    // nothing to do here
}

PostgreSQLRowIDBackEnd::~PostgreSQLRowIDBackEnd()
{
    // nothing to do here
}
