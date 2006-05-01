//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

Sqlite3RowIDBackEnd::Sqlite3RowIDBackEnd(
    Sqlite3SessionBackEnd & /* session */)
{
    // ...
}

Sqlite3RowIDBackEnd::~Sqlite3RowIDBackEnd()
{
    // ...
}
