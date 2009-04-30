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
#pragma warning(disable:4355 4996)
#endif

using namespace soci;
using namespace soci::details;


postgresql_session_backend::postgresql_session_backend(
    std::string const & connectString)
    : statementCount_(0)
{
    PGconn * conn = PQconnectdb(connectString.c_str());
    if (conn == NULL || PQstatus(conn) != CONNECTION_OK)
    {
        std::string msg = "Cannot establish connection to the database.";
        if (conn != NULL)
        {
            msg += '\n';
            msg += PQerrorMessage(conn);
            PQfinish(conn);
        }

        throw soci_error(msg);
    }

    conn_ = conn;
}

postgresql_session_backend::~postgresql_session_backend()
{
    clean_up();
}

namespace // unnamed
{

// helper function for hardoded queries
void hard_exec(PGconn * conn, char const * query, char const * errMsg)
{
    PGresult * result = PQexec(conn, query);

    if (result == NULL)
    {
        throw soci_error(errMsg);
    }

    ExecStatusType const status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
    {
        throw soci_error(PQresultErrorMessage(result));
    }

    PQclear(result);
}

} // namespace unnamed

void postgresql_session_backend::begin()
{
    hard_exec(conn_, "BEGIN", "Cannot begin transaction.");
}

void postgresql_session_backend::commit()
{
    hard_exec(conn_, "COMMIT", "Cannot commit transaction.");
}

void postgresql_session_backend::rollback()
{
    hard_exec(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void postgresql_session_backend::clean_up()
{
    if (conn_ != NULL)
    {
        PQfinish(conn_);
        conn_ = NULL;
    }
}

std::string postgresql_session_backend::get_next_statement_name()
{
    char nameBuf[20]; // arbitrary length
    sprintf(nameBuf, "st_%d", ++statementCount_);
    return nameBuf;
}

postgresql_statement_backend * postgresql_session_backend::make_statement_backend()
{
    return new postgresql_statement_backend(*this);
}

postgresql_rowid_backend * postgresql_session_backend::make_rowid_backend()
{
    return new postgresql_rowid_backend(*this);
}

postgresql_blob_backend * postgresql_session_backend::make_blob_backend()
{
    return new postgresql_blob_backend(*this);
}
