//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"

#include "soci/connection-parameters.h"

#include "soci-cstrtoi.h"

#include <functional>
#include <memory>
#include <sqlite3.h>
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

// Callback function used to construct the error message in the provided stream.
//
// SQLite3 own error message will be appended to it.
using error_callback = std::function<void (std::ostream& ostr)>;

// helper function for hardcoded queries: this is a simple wrapper for
// sqlite3_exec() which throws an exception on error.
void execute_hardcoded(sqlite_api::sqlite3* conn, char const* const query,
                       error_callback const& errCallback,
                       int (*callback)(void*, int, char**, char**) = NULL,
                       void* callback_arg = NULL)
{
    char *zErrMsg = 0;
    int const res = sqlite3_exec(conn, query, callback, callback_arg, &zErrMsg);

    std::unique_ptr<char, void(*)(void*)> zErrMsgPtr(zErrMsg, sqlite3_free);
    if (res != SQLITE_OK)
    {
        std::ostringstream ss;
        errCallback(ss);

        throw sqlite3_soci_error(conn, ss.str(), zErrMsg);
    }
}

// Simpler to use overload which uses a hard coded error message.
void execute_hardcoded(sqlite_api::sqlite3* conn, char const* const query, char const* const errMsg,
                       int (*callback)(void*, int, char**, char**) = NULL,
                       void* callback_arg = NULL)
{
    return execute_hardcoded(conn, query,
        [errMsg](std::ostream& ostr) { ostr << errMsg; },
        callback, callback_arg
    );
}

void check_sqlite_err(sqlite_api::sqlite3* conn, int res,
                      error_callback const& errCallback)
{
    if (SQLITE_OK != res)
    {
        std::ostringstream ss;
        errCallback(ss);

        sqlite3_soci_error const error(conn, ss.str());
        sqlite3_close(conn); // connection must be closed here
        throw error;
    }
}

void check_sqlite_err(sqlite_api::sqlite3* conn, int res, char const* const errMsg)
{
    return check_sqlite_err(conn, res, [errMsg](std::ostream& ostr) { ostr << errMsg; });
}

} // namespace anonymous

static int sequence_table_exists_callback(void* ctxt, int result_columns, char**, char**)
{
    bool* const flag = static_cast<bool*>(ctxt);
    *flag = result_columns > 0;
    return 0;
}

static bool check_if_sequence_table_exists(sqlite_api::sqlite3* conn)
{
    bool sequence_table_exists = false;
    execute_hardcoded
    (
      conn,
      "select name from sqlite_master where type='table' and name='sqlite_sequence'",
      "Failed checking if the sqlite_sequence table exists",
      &sequence_table_exists_callback,
      &sequence_table_exists
    );

    return sequence_table_exists;
}

sqlite3_session_backend::sqlite3_session_backend(
    connection_parameters const & parameters)
    : sequence_table_exists_(false)
{
    int timeout = 0;
    int connection_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string vfs;
    std::string synchronous;
    std::string foreignKeys;
    std::string const & connectString = parameters.get_connect_string();
    std::string dbname;

    auto params = parameters;
    if (connectString.find('=') == std::string::npos)
    {
        // The entire connection string must be just the database name.
        dbname = connectString;
    }
    else
    {
        params.extract_options_from_space_separated_string();
    }

    std::string val;
    if (params.get_option("dbname", val) || params.get_option("db", val))
    {
        dbname = val;
    }
    if (params.get_option("timeout", val))
    {
        std::istringstream converter(val);
        converter >> timeout;
    }
    if (params.get_option("synchronous", val))
    {
        synchronous = val;
    }
    if (params.is_option_on("readonly"))
    {
        connection_flags = (connection_flags | SQLITE_OPEN_READONLY) & ~(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    }
    if (params.is_option_on("nocreate"))
    {
        connection_flags &= ~SQLITE_OPEN_CREATE;
    }
    if (params.is_option_on("shared_cache"))
    {
        connection_flags |= SQLITE_OPEN_SHAREDCACHE;
    }
    params.get_option("vfs", vfs);
    params.get_option("foreign_keys", foreignKeys);

    if (dbname.empty())
    {
        throw soci_error("Database name must be specified");
    }

    int res = sqlite3_open_v2(dbname.c_str(), &conn_, connection_flags, (vfs.empty()?NULL:vfs.c_str()));
    check_sqlite_err(conn_, res,
        [&dbname](std::ostream& ostr)
        {
            ostr << "Cannot establish connection to \"" << dbname << "\"";
        }
    );

    // Set the timeout first to have effect on the following queries.
    res = sqlite3_busy_timeout(conn_, timeout * 1000);
    check_sqlite_err(conn_, res, "Failed to set busy timeout for connection. ");

    if (!synchronous.empty())
    {
        std::string const query("pragma synchronous=" + synchronous);
        execute_hardcoded(conn_, query.c_str(),
            [&synchronous](std::ostream& ostr)
            {
                ostr << "Setting synchronous pragma to \"" << synchronous << "\" failed";
            }
        );
    }

    if (!foreignKeys.empty())
    {
        std::string const query("pragma foreign_keys=" + foreignKeys);
        execute_hardcoded(conn_, query.c_str(),
            [&foreignKeys](std::ostream& ostr)
            {
                ostr << "Setting foreign_keys pragma to \"" << foreignKeys << "\" failed";
            }
        );
    }
}

sqlite3_session_backend::~sqlite3_session_backend()
{
    clean_up();
}

void sqlite3_session_backend::begin()
{
    execute_hardcoded(conn_, "BEGIN", "Cannot begin transaction.");
}

void sqlite3_session_backend::commit()
{
    execute_hardcoded(conn_, "COMMIT", "Cannot commit transaction.");
}

void sqlite3_session_backend::rollback()
{
    execute_hardcoded(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

// Argument passed to store_single_value_callback(), which is used to retrieve
// a single numeric value from a hardcoded query.
struct single_value_callback_ctx
{
    single_value_callback_ctx() : valid_(false) {}

    long long value_;
    bool valid_;
};

static int store_single_value_callback(void* ctx, int result_columns, char** values, char**)
{
    single_value_callback_ctx* arg = static_cast<single_value_callback_ctx*>(ctx);

    if (result_columns == 1 && values[0])
    {
        if (cstring_to_integer(arg->value_, values[0]))
            arg->valid_ = true;
    }

    return 0;
}

static std::string sanitize_table_name(std::string const& table)
{
    std::string ret;
    ret.reserve(table.length());
    for (std::string::size_type pos = 0; pos < table.size(); ++pos)
    {
        if (isspace(table[pos]))
            throw soci_error("Table name must not contain whitespace");
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
    single_value_callback_ctx ctx;
    if (sequence_table_exists_ || check_if_sequence_table_exists(conn_))
    {
        // Once the sqlite_sequence table is created (because of a column marked AUTOINCREMENT)
        // it can never be dropped, so don't search for it again.
        sequence_table_exists_ = true;

        std::string const query =
            "select seq from sqlite_sequence where name ='" + sanitize_table_name(table) + "'";
        execute_hardcoded(conn_, query.c_str(),  "Unable to get value in sqlite_sequence",
                          &store_single_value_callback, &ctx);

        // The value will not be filled if the callback was never called.
        // It may mean either that nothing was inserted yet into the table
        // that has an AUTOINCREMENT column, or that the table does not have an AUTOINCREMENT
        // column.
        if (ctx.valid_)
        {
            value = ctx.value_;
            return true;
        }
    }

    // Fall-back just get the maximum rowid of what was already inserted in the
    // table. This has the disadvantage that if rows were deleted, then ids may be re-used.
    // But, if one cares about that, AUTOINCREMENT should be used anyway.
    std::string const max_rowid_query = "select max(rowid) from \"" + sanitize_table_name(table) + "\"";
    execute_hardcoded(conn_, max_rowid_query.c_str(),  "Unable to get max rowid",
                      &store_single_value_callback, &ctx);
    value = ctx.valid_ ? ctx.value_ : 0;

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

// static
const char* sqlite3_session_backend::libversion()
{
    return sqlite3_libversion();
}

// static
int sqlite3_session_backend::libversion_number()
{
    return sqlite3_libversion_number();
}
