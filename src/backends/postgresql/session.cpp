//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/soci-platform.h"
#include "soci/postgresql/soci-postgresql.h"
#include "soci/session.h"
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

using namespace soci;
using namespace soci::details;

namespace // unnamed
{

// helper function for hardcoded queries
void hard_exec(postgresql_session_backend & session_backend,
    PGconn * conn, char const * query, char const * errMsg)
{
    postgresql_result(session_backend, PQexec(conn, query)).check_for_errors(errMsg);
}

// helper function to quote a string before sinding to PostgreSQL
char * quote(PGconn * conn, const char *s, size_t len)
{
    int error_code;
    char *retv = new char[2 * len + 3];
    retv[0] = '\'';
    int len_esc = PQescapeStringConn(conn, retv + 1, s, len, &error_code);
    if (error_code > 0)
    {
        len_esc = 0;
    }
    retv[len_esc + 1] = '\'';
    retv[len_esc + 2] = '\0';

    return retv;
}

// helper function to collect schemas from search_path
std::vector<std::string> get_schema_names(PGconn * conn)
{
    std::vector<std::string> schema_names;
    PGresult* search_path = PQexec(conn, "SHOW search_path");
    if (PQresultStatus(search_path) == PGRES_TUPLES_OK)
    {
        if (PQntuples(search_path) > 0)
        {
            std::string search_path_content = PQgetvalue(search_path, 0, 0);

            bool quoted = false;
	    std::string schema;
            while (!search_path_content.empty())
	    {
                switch (search_path_content[0])
                {
                case '"':
                    quoted = !quoted;
                    break;
                case ',':
                case ' ':
                    if (!quoted)
                    {
                        if (search_path_content[0] == ',')
                        {
                            schema_names.push_back(schema);
                            schema = "";
                        }
                        break;
                    }
                    BOOST_FALLTHROUGH;
                default:
                    schema.push_back(search_path_content[0]);
                }
                search_path_content.erase(search_path_content.begin());
            }
            if (!schema.empty())
                schema_names.push_back(schema);
	    for (std::string& schema_name: schema_names)
            {
                if (schema_name == "$user")
                {
                    PGresult* current_user = PQexec(conn, "SELECT current_user");
                    if (PQresultStatus(current_user) == PGRES_TUPLES_OK)
                    {
                        if (PQntuples(current_user) > 0)
                        {
                            schema_name = PQgetvalue(current_user, 0, 0);
                        }
                    }
                }

                // Assure no bad characters
                char * escaped_schema = quote(conn, schema_name.c_str(), schema_name.length());
                schema_name = escaped_schema;
                delete[] escaped_schema;
            }
        }
    }
    PQclear(search_path);
    if (schema_names.empty())
    {
        PGresult* current_user = PQexec(conn, "SELECT current_user");
        if (PQresultStatus(current_user) == PGRES_TUPLES_OK)
        {
            if (PQntuples(current_user) > 0)
            {
                std::string user = PQgetvalue(current_user, 0, 0);

                // Assure no bad characters
                char * escaped_user = quote(conn, user.c_str(), user.length());
                schema_names.push_back(escaped_user);
                delete[] escaped_user;
            }
        }
        schema_names.push_back("public");
    }

    return schema_names;
}

// helper function to create a comma separated list of strings
std::string create_list_of_strings(const std::vector<std::string>& list)
{
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i != 0) {
            oss << ", ";
        }
        oss << list[i];
    }
    return oss.str();
}

// helper function to create a case list for strings
std::string create_case_list_of_strings(const std::vector<std::string>& list)
{
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); ++i) {
        oss << " WHEN " << list[i] << " THEN " << i;
    }
    return oss.str();
}

} // namespace unnamed

postgresql_session_backend::postgresql_session_backend(
    connection_parameters const& parameters, bool single_row_mode)
    : statementCount_(0), conn_(0)
{
    single_row_mode_ = single_row_mode;

    connect(parameters);
}

void postgresql_session_backend::connect(
    connection_parameters const& parameters)
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

    // Increase the number of digits used for floating point values to ensure
    // that the conversions to/from text round trip correctly, which is not the
    // case with the default value of 0. Use the maximal supported value, which
    // was 2 until 9.x and is 3 since it.
    int const version = PQserverVersion(conn);
    hard_exec(*this, conn,
        version >= 90000 ? "SET extra_float_digits = 3"
                         : "SET extra_float_digits = 2",
        "Cannot set extra_float_digits parameter");

    conn_ = conn;
    connectionParameters_ = parameters;
}

postgresql_session_backend::~postgresql_session_backend()
{
    clean_up();
}

bool postgresql_session_backend::is_connected()
{
    // For the connection to work, its status must be OK, but this is not
    // sufficient, so try to actually do something with it, even if it's
    // something as trivial as sending an empty command to the server.
    if ( PQstatus(conn_) != CONNECTION_OK )
        return false;

    postgresql_result(*this, PQexec(conn_, "/* ping */"));

    // And then check it again.
    return PQstatus(conn_) == CONNECTION_OK;
}

void postgresql_session_backend::begin()
{
    hard_exec(*this, conn_, "BEGIN", "Cannot begin transaction.");
}

void postgresql_session_backend::commit()
{
    hard_exec(*this, conn_, "COMMIT", "Cannot commit transaction.");
}

void postgresql_session_backend::rollback()
{
    hard_exec(*this, conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void postgresql_session_backend::deallocate_prepared_statement(
    const std::string & statementName)
{
    const std::string & query = "DEALLOCATE " + statementName;

    hard_exec(*this, conn_, query.c_str(),
        "Cannot deallocate prepared statement.");
}

bool postgresql_session_backend::get_next_sequence_value(
    session & s, std::string const & sequence, long long & value)
{
    s << "select nextval('" + sequence + "')", into(value);

    return true;
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
    return new postgresql_statement_backend(*this, single_row_mode_);
}

postgresql_rowid_backend * postgresql_session_backend::make_rowid_backend()
{
    return new postgresql_rowid_backend(*this);
}

postgresql_blob_backend * postgresql_session_backend::make_blob_backend()
{
    return new postgresql_blob_backend(*this);
}

std::string postgresql_session_backend::get_table_names_query() const
{
    return std::string("SELECT table_schema || '.' || table_name AS \"TABLE_NAME\" FROM information_schema.tables WHERE table_schema in (") + create_list_of_strings(get_schema_names(conn_)) + ")";
}

std::string postgresql_session_backend::get_column_descriptions_query() const
{
    std::vector<std::string> schema_list = get_schema_names(conn_);
    return std::string("WITH Schema AS ("
           " SELECT table_schema"
           " FROM information_schema.columns"
           " WHERE table_name = :t"
           " AND CASE"
           " WHEN :s::VARCHAR is not NULL THEN table_schema = :s::VARCHAR"
           " ELSE table_schema in (") + create_list_of_strings(schema_list) + ") END"
           " ORDER BY"
           " CASE table_schema" +
           create_case_list_of_strings(schema_list) +
           " ELSE " + std::to_string(schema_list.size()) + " END"
           " LIMIT 1 )"
           " SELECT column_name as \"COLUMN_NAME\","
           " data_type as \"DATA_TYPE\","
           " character_maximum_length as \"CHARACTER_MAXIMUM_LENGTH\","
           " numeric_precision as \"NUMERIC_PRECISION\","
           " numeric_scale as \"NUMERIC_SCALE\","
           " is_nullable as \"IS_NULLABLE\""
           " FROM information_schema.columns"
           " WHERE table_name = :t"
           " AND table_schema = ("
           " SELECT table_schema"
           " FROM Schema )";
}
