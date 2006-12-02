//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include <soci.h>
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
#pragma warning(disable:4355 4996)
#endif

using namespace SOCI;
using namespace SOCI::details;


PostgreSQLSessionBackEnd::PostgreSQLSessionBackEnd(
    std::string const & connectString)
    : statementCount_(0)
{
    conn_ = PQconnectdb(connectString.c_str());
    if (conn_ == NULL || PQstatus(conn_) != CONNECTION_OK)
    {
        throw SOCIError("Cannot establish connection to the database.");
    }
}

PostgreSQLSessionBackEnd::~PostgreSQLSessionBackEnd()
{
    cleanUp();
}

namespace // anonymous
{

// helper function for hardoded queries
void hardExec(PGconn *conn, char const *query, char const *errMsg)
{
    PGresult *result = PQexec(conn, query);

    if (result == NULL)
    {
        throw SOCIError(errMsg);
    }

    ExecStatusType status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
    {
        throw SOCIError(PQresultErrorMessage(result));
    }

    PQclear(result);
}

} // namespace anonymous

void PostgreSQLSessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN", "Cannot begin transaction.");
}

void PostgreSQLSessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT", "Cannot commit transaction.");
}

void PostgreSQLSessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void PostgreSQLSessionBackEnd::cleanUp()
{
    if (conn_ != NULL)
    {
        PQfinish(conn_);
        conn_ = NULL;
    }
}

std::string PostgreSQLSessionBackEnd::getNextStatementName()
{
    char nameBuf[20]; // arbitrary length
    sprintf(nameBuf, "st_%d", ++statementCount_);
    return nameBuf;
}

PostgreSQLStatementBackEnd * PostgreSQLSessionBackEnd::makeStatementBackEnd()
{
    return new PostgreSQLStatementBackEnd(*this);
}

PostgreSQLRowIDBackEnd * PostgreSQLSessionBackEnd::makeRowIDBackEnd()
{
    return new PostgreSQLRowIDBackEnd(*this);
}

PostgreSQLBLOBBackEnd * PostgreSQLSessionBackEnd::makeBLOBBackEnd()
{
    return new PostgreSQLBLOBBackEnd(*this);
}
