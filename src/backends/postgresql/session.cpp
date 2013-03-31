//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include "error.h"
#include <connection-parameters.h>
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef SOCI_POSTGRESQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_POSTGRESQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355 4996)
#endif

#ifndef WIN32
#include <unistd.h> // for sleep()
#else
namespace {
void sleep(unsigned duration) { _sleep(duration*1000); }
} // namespace
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::postgresql;

postgresql_session_backend::postgresql_session_backend(
    connection_parameters const& parameters)
    : statementCount_(0), disconnected_(false)
{
    PGconn* conn = PQconnectdb(parameters.get_connect_string().c_str());
    if (0 == conn || CONNECTION_OK != PQstatus(conn))
    {
        std::string msg = "Cannot establish connection to the database.";
        if (0 != conn)
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

// helper function for hardcoded queries
void hard_exec(postgresql_session_backend& session, char const * query, char const * errMsg)
{
    PGresult* result = 0;
    do {
        result = PQexec(session.conn_, query);
    } while(session.check_connection(result));
    if (0 == result)
    {
        throw soci_error(errMsg);
    }

    ExecStatusType const status = PQresultStatus(result);
    if (PGRES_COMMAND_OK != status)
    {
        // releases result with PQclear
        throw_postgresql_soci_error(result);
    }

    PQclear(result);
}

} // namespace unnamed

void postgresql_session_backend::begin()
{
    hard_exec(*this, "BEGIN", "Cannot begin transaction.");
}

void postgresql_session_backend::commit()
{
    hard_exec(*this, "COMMIT", "Cannot commit transaction.");
}

void postgresql_session_backend::rollback()
{
    hard_exec(*this, "ROLLBACK", "Cannot rollback transaction.");
}

void postgresql_session_backend::deallocate_prepared_statement(
    const std::string & statementName)
{
    const std::string & query = "DEALLOCATE " + statementName;

    hard_exec(*this, query.c_str(),
        "Cannot deallocate prepared statement.");
}

void postgresql_session_backend::clean_up()
{
    if (0 != conn_)
    {
        PQfinish(conn_);
        conn_ = 0;
    }
}

std::string postgresql_session_backend::get_next_statement_name()
{
    char nameBuf[20] = { 0 }; // arbitrary length
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

bool postgresql_session_backend::check_connection(PGresult*& res, int timeout /*= 10*/)
{
    if (!disconnected_)
    {
        // First, we check if the connection was 
        // really lost (it may be even a success)
        if (res != NULL && PQresultStatus(res) == PGRES_COMMAND_OK)
        {
            return false;
        }
        if (PQstatus(conn_) != CONNECTION_BAD)
        {
            return false;
        }
    }
    // Try at least once
    do {
        std::time_t start = std::time(NULL);
        // Reconnecting... now
        PQreset(conn_);
        if (PQstatus(conn_) != CONNECTION_BAD)
        {
            disconnected_ = false;
            // Done, clean results for the next attempt
            PQclear(res);
            return true;
        }
        // How much time did we spend in this last attempt?
        int time_spent = static_cast<int>(std::time(NULL) - start);
        // Less than 1 second? Give the server a break
        if (time_spent == 0)
        {
            sleep(1);
            time_spent = 1;
        }
        timeout -= time_spent;
    // Do we have time for another try?
    } while (timeout > 0);

    // We did everything we could
    // Let it with the original error
    // Mark the session as disconnected
    disconnected_ = true;
    return false;
}
