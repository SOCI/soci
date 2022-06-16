//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"

#include "soci/connection-parameters.h"

#include "soci-cstrtoi.h"

#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace sqlite_api;

namespace // anonymous
{

// helper function for hardcoded queries
void execude_hardcoded(sqlite_api::sqlite3* conn, char const* const query, char const* const errMsg)
{
    char *zErrMsg = 0;
    int const res = sqlite3_exec(conn, query, 0, 0, &zErrMsg);
    if (res != SQLITE_OK)
    {
        std::ostringstream ss;
        ss << errMsg << " " << zErrMsg;
        sqlite3_free(zErrMsg);
        throw sqlite3_soci_error(ss.str(), res);
    }
}

void check_sqlite_err(sqlite_api::sqlite3* conn, int res, char const* const errMsg)
{
    if (SQLITE_OK != res)
    {
        const char *zErrMsg = sqlite3_errmsg(conn);
        std::ostringstream ss;
        ss << errMsg << zErrMsg;
        sqlite3_close(conn); // connection must be closed here
        throw sqlite3_soci_error(ss.str(), res);
    }
}

void check_sqlite_err_keep_conn(sqlite_api::sqlite3* conn, int res, char const* const errMsg)
{
    if (SQLITE_OK != res)
    {
        const char *zErrMsg = sqlite3_errmsg(conn);
        std::ostringstream ss;
        ss << errMsg << zErrMsg;
        throw sqlite3_soci_error(ss.str(), res);
    }
}

} // namespace anonymous

static int CheckSequenceTableCallback(void* ctxt, int valueNum, char**, char**)
{
    bool* flag = (bool*)ctxt;
    *flag = valueNum > 0;
    return 0;
}

static bool SequenceTableExists(sqlite_api::sqlite3* conn)
{
    bool sequenceTableExists = false;
    std::string query = "select name from sqlite_master where type='table' and name='sqlite_sequence'";
    int const res = sqlite3_exec(conn, query.c_str(), &CheckSequenceTableCallback,
                                 &sequenceTableExists,
                                 NULL);
    check_sqlite_err_keep_conn(conn, res, "Failed checking if the sqlite_sequence table exists");

    return sequenceTableExists;
}

sqlite3_session_backend::sqlite3_session_backend(
    connection_parameters const & parameters)
    : sequenceTableExists_(false)
{
    int timeout = 0;
    int connection_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string vfs;
    std::string synchronous;
    std::string foreignKeys;
    std::string const & connectString = parameters.get_connect_string();
    std::string dbname(connectString);
    std::stringstream ssconn(connectString);
    while (!ssconn.eof() && ssconn.str().find('=') != std::string::npos)
    {
        std::string key, val;
        std::getline(ssconn, key, '=');
        std::getline(ssconn, val, ' ');

        if (val.size()>0 && val[0]=='\"')
        {
            std::string quotedVal = val.erase(0, 1);

            if (quotedVal[quotedVal.size()-1] ==  '\"')
            {
                quotedVal.erase(val.size()-1);
            }
            else // space inside value string
            {
                std::getline(ssconn, val, '\"');
                quotedVal = quotedVal + " " + val;
                std::string keepspace;
                std::getline(ssconn, keepspace, ' ');
            }

            val = quotedVal;
        }

        if ("dbname" == key || "db" == key)
        {
            dbname = val;
        }
        else if ("timeout" == key)
        {
            std::istringstream converter(val);
            converter >> timeout;
        }
        else if ("synchronous" == key)
        {
            synchronous = val;
        }
        else if ("readonly" == key)
        {
            connection_flags = (connection_flags | SQLITE_OPEN_READONLY) & ~(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        }
        else if ("shared_cache" == key && "true" == val)
        {
            connection_flags |= SQLITE_OPEN_SHAREDCACHE;
        }
        else if ("vfs" == key)
        {
            vfs = val;
        }
        else if ("foreign_keys" == key)
        {
            foreignKeys = val;
        }
    }

    int res = sqlite3_open_v2(dbname.c_str(), &conn_, connection_flags, (vfs.empty()?NULL:vfs.c_str()));
    check_sqlite_err(conn_, res, "Cannot establish connection to the database. ");

    if (!synchronous.empty())
    {
        std::string const query("pragma synchronous=" + synchronous);
        std::string const errMsg("Query failed: " + query);
        execude_hardcoded(conn_, query.c_str(), errMsg.c_str());
    }

    if (!foreignKeys.empty())
    {
        std::string const query("pragma foreign_keys=" + foreignKeys);
        std::string const errMsg("Executing query: " + query + " failed");
        execude_hardcoded(conn_, query.c_str(), errMsg.c_str());
    }

    res = sqlite3_busy_timeout(conn_, timeout * 1000);
    check_sqlite_err(conn_, res, "Failed to set busy timeout for connection. ");
}

sqlite3_session_backend::~sqlite3_session_backend()
{
    clean_up();
}

void sqlite3_session_backend::begin()
{
    execude_hardcoded(conn_, "BEGIN", "Cannot begin transaction.");
}

void sqlite3_session_backend::commit()
{
    execude_hardcoded(conn_, "COMMIT", "Cannot commit transaction.");
}

void sqlite3_session_backend::rollback()
{
    execude_hardcoded(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

struct sequence_context
{
    long long value_;
    bool valid_;
};

static int get_one_long(void* ctx, int valueNum, char** values, char**)
{
    sequence_context* seq = static_cast<sequence_context*>(ctx);
    // Callback is called only in the case where there is at least
    // one result tuple. Initialize the resulting value and
    // mark that the callback was called.
    seq->value_ = 0;
    seq->valid_ = true;
    if (valueNum == 1 && values[0])
        if (!cstring_to_integer(seq->value_, values[0]))
            seq->value_ = 0;

    return 0;
}

static std::string sanitize_table_name(std::string const& table)
{
    std::string ret;
    for (std::string::size_type pos = 0; pos < table.size(); ++pos)
    {
        if (isspace(table[pos]))
            throw sqlite3_soci_error("Table name must not contain whitespace", 0);
        const char c = table[pos];
        ret += c;
        if (c == '\'')
            ret += '\'';
        else if (c == '\"')
            ret += '\"';
    }
    return ret;
}

bool sqlite3_session_backend::get_last_insert_id(
    session &, std::string const & table, long long & value)
{
    sequence_context seqCtxt;
    seqCtxt.valid_ = false;
    seqCtxt.value_ = 0;
    if (sequenceTableExists_ || SequenceTableExists(conn_))
    {
        // Once the sqlite_sequence table is created (because of a column marked AUTOINCREMENT)
        // it can never be dropped, so don't search for it again.
        sequenceTableExists_ = true;

        std::string const query =
            "select seq from sqlite_sequence where name ='" + sanitize_table_name(table) + "'";
        int const res = sqlite3_exec(conn_, query.c_str(), &get_one_long, &seqCtxt, NULL);
        check_sqlite_err_keep_conn(conn_, res, "Unable to get value in sqlite_sequence");

        // The value will not be filled if the callback was never called.
        // It may mean either that nothing was inserted yet into the table
        // that has an AUTOINCREMENT column, or that the table does not have an AUTOINCREMENT
        // column.
        if (seqCtxt.valid_)
        {
            value = seqCtxt.value_;
            return true;
        }
    }

    // Fall-back just get the maximum rowid of what was already inserted in the
    // table. This has the disadvantage that if rows were deleted, then ids may be re-used.
    // But, if one cares about that, AUTOINCREMENT should be used anyway.
    std::string const maxRowIdQuery = "select max(rowid) from \"" + sanitize_table_name(table) + "\"";
    int const res = sqlite3_exec(conn_, maxRowIdQuery.c_str(), &get_one_long, &seqCtxt, NULL);
    check_sqlite_err_keep_conn(conn_, res, "Unable to get max rowid");
    value = seqCtxt.value_;

    return true;
}

void sqlite3_session_backend::clean_up()
{
    sqlite3_close(conn_);
}

sqlite3_statement_backend * sqlite3_session_backend::make_statement_backend()
{
    return new sqlite3_statement_backend(*this);
}

sqlite3_rowid_backend * sqlite3_session_backend::make_rowid_backend()
{
    return new sqlite3_rowid_backend(*this);
}

sqlite3_blob_backend * sqlite3_session_backend::make_blob_backend()
{
    return new sqlite3_blob_backend(*this);
}
