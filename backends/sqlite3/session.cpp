//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace sqlite_api;

namespace // anonymous
{

// helper function for hardcoded queries
void hardExec(sqlite_api::sqlite3 *conn, char const *query, char const *errMsg)
{
    char *zErrMsg = 0;
    int res = sqlite3_exec(conn, query, 0, 0, &zErrMsg);
    if (res != SQLITE_OK)
    {
        std::ostringstream ss;
        ss << errMsg << " "
           << zErrMsg;

        sqlite3_free(zErrMsg);
        
        throw SOCIError(ss.str());
    }
}

} // namespace anonymous


Sqlite3SessionBackEnd::Sqlite3SessionBackEnd(
    std::string const & connectString)
{
    int res;
    res = sqlite3_open(connectString.c_str(), &conn_);
    if (res != SQLITE_OK)
    {
        const char *zErrMsg = sqlite3_errmsg(conn_);

        std::ostringstream ss;
        ss << "Cannot establish connection to the database. "
           << zErrMsg;
        
        throw SOCIError(ss.str());
    }
}

Sqlite3SessionBackEnd::~Sqlite3SessionBackEnd()
{
    cleanUp();
}

void Sqlite3SessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN", "Cannot begin transaction.");    
}

void Sqlite3SessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT", "Cannot commit transaction.");
}

void Sqlite3SessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void Sqlite3SessionBackEnd::cleanUp()
{
    sqlite3_close(conn_);
}

Sqlite3StatementBackEnd * Sqlite3SessionBackEnd::makeStatementBackEnd()
{
    return new Sqlite3StatementBackEnd(*this);
}

Sqlite3RowIDBackEnd * Sqlite3SessionBackEnd::makeRowIDBackEnd()
{
    return new Sqlite3RowIDBackEnd(*this);
}

Sqlite3BLOBBackEnd * Sqlite3SessionBackEnd::makeBLOBBackEnd()
{
    return new Sqlite3BLOBBackEnd(*this);
}
