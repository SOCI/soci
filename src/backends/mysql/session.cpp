//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"
#include "soci/connection-parameters.h"
// std
#include <cctype>
#include <cerrno>
#include <ciso646>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using std::string;


namespace
{ // anonymous

bool valid_int(const string & s)
{
    char *tail;
    const char *cstr = s.c_str();
    errno = 0;
    long n = std::strtol(cstr, &tail, 10);
    if (errno != 0 or n > INT_MAX or n < INT_MIN)
    {
        return false;
    }
    if (*tail != '\0')
    {
        return false;
    }
    return true;
}

} // namespace anonymous


mysql_session_backend::mysql_session_backend(
    connection_parameters const & params)
{
    string host, user, password, db, unix_socket, portstr, ssl_ca, ssl_cert, ssl_key,
        charset, local_infstr;
    int port = 0, local_infile = 0;

    bool host_p = params.get_option("host", host);
    bool user_p = params.get_option("user", user);
    bool password_p = (params.get_option("pass", password) ||
            params.get_option("password", password));
    bool db_p = ( params.get_option("db", db) ||
            params.get_option("dbname", db) ||
            params.get_option("service", db) );
    bool unix_socket_p = params.get_option("unix_socket", unix_socket);
    bool port_p = params.get_option("port", portstr);
    if (port_p)
    {
        if (valid_int(portstr))
        {
            port = std::atoi(portstr.c_str());
            if (port < 0)
            {
                throw soci_error("Wrong port value");
            }
        }
        else
        {
            throw soci_error("Port value must be a number");
        }
    }

    bool ssl_ca_p = params.get_option("sslca", ssl_ca);
    bool ssl_cert_p = params.get_option("sslcert", ssl_cert);
    bool ssl_key_p = params.get_option("sslkey", ssl_key);
    bool local_infile_p = params.get_option("local_infile", local_infstr);
    if (local_infile_p)
    {
        if (valid_int(local_infstr))
        {
            local_infile = std::atoi(local_infstr.c_str());
            if (local_infile < 0 || local_infile > 1)
            {
                throw soci_error("Wrong local_infile parameter");
            }
        }
        else
        {
            throw soci_error("Wrong local_infile parameter");
        }
    }
    bool charset_p = params.get_option("charset", charset);

    conn_ = mysql_init(NULL);
    if (conn_ == NULL)
    {
        throw soci_error("mysql_init() failed.");
    }
    if (charset_p)
    {
        if (0 != mysql_options(conn_, MYSQL_SET_CHARSET_NAME, charset.c_str()))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_SET_CHARSET_NAME) failed.");
        }
    }
    if (ssl_ca_p)
    {
        mysql_ssl_set(conn_, ssl_key_p ? ssl_key.c_str() : NULL,
                      ssl_cert_p ? ssl_cert.c_str() : NULL,
                      ssl_ca.c_str(), 0, 0);
    }
    if (local_infile_p and local_infile == 1)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_LOCAL_INFILE, NULL))
        {
            clean_up();
            throw soci_error(
                "mysql_options() failed when trying to set local-infile.");
        }
    }
    if (mysql_real_connect(conn_,
            host_p ? host.c_str() : NULL,
            user_p ? user.c_str() : NULL,
            password_p ? password.c_str() : NULL,
            db_p ? db.c_str() : NULL,
            port_p ? port : 0,
            unix_socket_p ? unix_socket.c_str() : NULL,
#ifdef CLIENT_MULTI_RESULTS
            CLIENT_FOUND_ROWS | CLIENT_MULTI_RESULTS) == NULL)
#else
            CLIENT_FOUND_ROWS) == NULL)
#endif
    {
        string errMsg = mysql_error(conn_);
        unsigned int errNum = mysql_errno(conn_);
        clean_up();
        throw mysql_soci_error(errMsg, errNum);
    }
}


mysql_session_backend::~mysql_session_backend()
{
    clean_up();
}

namespace // unnamed
{

// helper function for hardcoded queries
void hard_exec(MYSQL *conn, const string & query)
{
    if (0 != mysql_real_query(conn, query.c_str(),
            static_cast<unsigned long>(query.size())))
    {
        throw soci_error(mysql_error(conn));
    }
}

} // namespace unnamed

void mysql_session_backend::begin()
{
    hard_exec(conn_, "BEGIN");
}

void mysql_session_backend::commit()
{
    hard_exec(conn_, "COMMIT");
}

void mysql_session_backend::rollback()
{
    hard_exec(conn_, "ROLLBACK");
}

bool mysql_session_backend::get_last_insert_id(
    session & /* s */, std::string const & /* table */, long & value)
{
    value = static_cast<long>(mysql_insert_id(conn_));

    return true;
}

void mysql_session_backend::clean_up()
{
    if (conn_ != NULL)
    {
        mysql_close(conn_);
        conn_ = NULL;
    }
}

mysql_statement_backend * mysql_session_backend::make_statement_backend()
{
    return new mysql_statement_backend(*this);
}

mysql_rowid_backend * mysql_session_backend::make_rowid_backend()
{
    return new mysql_rowid_backend(*this);
}

mysql_blob_backend * mysql_session_backend::make_blob_backend()
{
    return new mysql_blob_backend(*this);
}
