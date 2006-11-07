//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci.h"
#include "soci-sqlite3.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


// concrete factory for Empty concrete strategies
Sqlite3SessionBackEnd * Sqlite3BackEndFactory::makeSession(
     std::string const &connectString) const
{
     return new Sqlite3SessionBackEnd(connectString);
}

SOCI_SQLITE3_DECL Sqlite3BackEndFactory const SOCI::sqlite3;
