//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"
#include "soci/connection-parameters.h"
// std
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace soci;
using namespace soci::details;
using std::string;

// SSL options existing in all supported MySQL versions but not in MariaDB.
#ifndef MARIADB_VERSION_ID
#define SOCI_HAS_MYSQL_SSL_OPT
#endif

namespace
{ // anonymous

// Helper class used to ensure we call mysql_library_init() before opening the
// first MySQL connection and mysql_library_end() on application shutdown.
class mysql_library
{
private:
    mysql_library()
    {
        if (mysql_library_init(0, NULL, NULL))
        {
            throw soci_error("Failed to initialize MySQL library.");
        }
    }

    ~mysql_library()
    {
        mysql_library_end();
    }

public:
    // This function only exists for its side effect of creating an instance of
    // this class.
    static void ensure_initialized()
    {
        static mysql_library ins;
    }
};

void skip_white(std::string::const_iterator *i,
    std::string::const_iterator const & end, bool endok)
{
    for (;;)
    {
        if (*i == end)
        {
            if (endok)
            {
                return;
            }
            else
            {
                throw soci_error("Unexpected end of connection string.");
            }
        }
        if (std::isspace(**i))
        {
            ++*i;
        }
        else
        {
            return;
        }
    }
}

std::string param_name(std::string::const_iterator *i,
    std::string::const_iterator const & end)
{
    std::string val("");
    for (;;)
    {
        if (*i == end || (!std::isalpha(**i) && **i != '_'))
        {
            break;
        }
        val += **i;
        ++*i;
    }
    return val;
}

string param_value(string::const_iterator *i,
    string::const_iterator const & end)
{
    string err = "Malformed connection string.";
    bool quot;
    if (**i == '\'')
    {
        quot = true;
        ++*i;
    }
    else
    {
        quot = false;
    }
    string val("");
    for (;;)
    {
        if (*i == end)
        {
            if (quot)
            {
                throw soci_error(err);
            }
            else
            {
                break;
            }
        }
        if (**i == '\'')
        {
            if (quot)
            {
                ++*i;
                break;
            }
            else
            {
                throw soci_error(err);
            }
        }
        if (!quot && std::isspace(**i))
        {
            break;
        }
        if (**i == '\\')
        {
            ++*i;
            if (*i == end)
            {
                throw soci_error(err);
            }
        }
        val += **i;
        ++*i;
    }
    return val;
}

bool valid_int(const string & s)
{
    char *tail;
    const char *cstr = s.c_str();
    errno = 0;
    long n = std::strtol(cstr, &tail, 10);
    if (errno != 0 || n > INT_MAX || n < INT_MIN)
    {
        return false;
    }
    if (*tail != '\0')
    {
        return false;
    }
    return true;
}

bool valid_uint(const string & s)
{
    char *tail;
    const char *cstr = s.c_str();
    errno = 0;
    unsigned long n = std::strtoul(cstr, &tail, 10);
    if (errno != 0 || n == 0 || n > UINT_MAX)
        return false;
    if (*tail != '\0')
        return false;
    return true;
}

void parse_connect_string(const string & connectString,
    string *host, bool *host_p,
    string *user, bool *user_p,
    string *password, bool *password_p,
    string *db, bool *db_p,
    string *unix_socket, bool *unix_socket_p,
    int *port, bool *port_p, string *ssl_ca, bool *ssl_ca_p,
    string *ssl_cert, bool *ssl_cert_p, string *ssl_key, bool *ssl_key_p,
    int *local_infile, bool *local_infile_p,
    string *charset, bool *charset_p, bool *reconnect_p,
    unsigned int *connect_timeout, bool *connect_timeout_p,
    unsigned int *read_timeout, bool *read_timeout_p,
    unsigned int *write_timeout, bool *write_timeout_p,
    unsigned int *ssl_mode, bool *ssl_mode_p)
{
    *host_p = false;
    *user_p = false;
    *password_p = false;
    *db_p = false;
    *unix_socket_p = false;
    *port = 0;
    *port_p = false;
    *ssl_ca_p = false;
    *ssl_cert_p = false;
    *ssl_key_p = false;
    *local_infile = 0;
    *local_infile_p = false;
    *charset_p = false;
    *reconnect_p = false;
    *connect_timeout_p = false;
    *read_timeout_p = false;
    *write_timeout_p = false;
    *ssl_mode_p = false;
    string err = "Malformed connection string.";
    string::const_iterator i = connectString.begin(),
        end = connectString.end();
    while (i != end)
    {
        skip_white(&i, end, true);
        if (i == end)
        {
            return;
        }
        string par = param_name(&i, end);
        skip_white(&i, end, false);
        if (*i == '=')
        {
            ++i;
        }
        else
        {
            throw soci_error(err);
        }
        skip_white(&i, end, false);
        string val = param_value(&i, end);
        if (par == "port" && !*port_p)
        {
            if (!valid_int(val))
            {
                throw soci_error(err);
            }
            *port = std::atoi(val.c_str());
            if (*port < 0)
            {
                throw soci_error(err);
            }
            *port_p = true;
        }
        else if (par == "host" && !*host_p)
        {
            *host = val;
            *host_p = true;
        }
        else if (par == "user" && !*user_p)
        {
            *user = val;
            *user_p = true;
        }
        else if ((par == "pass" || par == "password") && !*password_p)
        {
            *password = val;
            *password_p = true;
        }
        else if ((par == "db" || par == "dbname" || par == "service") && !*db_p)
        {
            *db = val;
            *db_p = true;
        }
        else if (par == "unix_socket" && !*unix_socket_p)
        {
            *unix_socket = val;
            *unix_socket_p = true;
        }
        else if (par == "sslca" && !*ssl_ca_p)
        {
            *ssl_ca = val;
            *ssl_ca_p = true;
        }
        else if (par == "sslcert" && !*ssl_cert_p)
        {
            *ssl_cert = val;
            *ssl_cert_p = true;
        }
        else if (par == "sslkey" && !*ssl_key_p)
        {
            *ssl_key = val;
            *ssl_key_p = true;
        }
        else if (par == "local_infile" && !*local_infile_p)
        {
            if (!valid_int(val))
            {
                throw soci_error(err);
            }
            *local_infile = std::atoi(val.c_str());
            if (*local_infile != 0 && *local_infile != 1)
            {
                throw soci_error(err);
            }
            *local_infile_p = true;
        } else if (par == "charset" && !*charset_p)
        {
            *charset = val;
            *charset_p = true;
        } else if (par == "reconnect" && !*reconnect_p)
        {
            if (val != "1")
                throw soci_error("\"reconnect\" option may only be set to 1");

            *reconnect_p = true;
        } else if (par == "connect_timeout" && !*connect_timeout_p)
        {
            if (!valid_uint(val))
                throw soci_error(err);
            char *endp;
            *connect_timeout = std::strtoul(val.c_str(), &endp, 10);
            *connect_timeout_p = true;
        } else if (par == "read_timeout" && !*read_timeout_p)
        {
            if (!valid_uint(val))
                throw soci_error(err);
            char *endp;
            *read_timeout = std::strtoul(val.c_str(), &endp, 10);
            *read_timeout_p = true;
        } else if (par == "write_timeout" && !*write_timeout_p)
        {
            if (!valid_uint(val))
                throw soci_error(err);
            char *endp;
            *write_timeout = std::strtoul(val.c_str(), &endp, 10);
            *write_timeout_p = true;
        } else if (par == "ssl_mode" && !*ssl_mode_p)
        {
#ifdef SOCI_HAS_MYSQL_SSL_OPT
            if (val=="DISABLED") *ssl_mode = SSL_MODE_DISABLED;
            else if (val=="PREFERRED") *ssl_mode = SSL_MODE_PREFERRED;
            else if (val=="REQUIRED") *ssl_mode = SSL_MODE_REQUIRED;
            else if (val=="VERIFY_CA") *ssl_mode = SSL_MODE_VERIFY_CA;
            else if (val=="VERIFY_IDENTITY") *ssl_mode = SSL_MODE_VERIFY_IDENTITY;
            else throw soci_error("\"ssl_mode\" setting is invalid");
            *ssl_mode_p = true;
#else
            SOCI_UNUSED(ssl_mode);
            throw soci_error("SSL options not supported with MariaDB");
#endif
        }
        else
        {
            throw soci_error(err);
        }
    }
}

} // namespace anonymous


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif

#if defined(__GNUC__) && ( __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 6)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif


mysql_session_backend::mysql_session_backend(
    connection_parameters const & parameters)
{
    mysql_library::ensure_initialized();

    string host, user, password, db, unix_socket, ssl_ca, ssl_cert, ssl_key,
        charset;
    int port, local_infile;
    unsigned int connect_timeout, read_timeout, write_timeout, ssl_mode;
    bool host_p, user_p, password_p, db_p, unix_socket_p, port_p,
        ssl_ca_p, ssl_cert_p, ssl_key_p, local_infile_p, charset_p, reconnect_p,
        connect_timeout_p, read_timeout_p, write_timeout_p, ssl_mode_p;
    parse_connect_string(parameters.get_connect_string(), &host, &host_p, &user, &user_p,
        &password, &password_p, &db, &db_p,
        &unix_socket, &unix_socket_p, &port, &port_p,
        &ssl_ca, &ssl_ca_p, &ssl_cert, &ssl_cert_p, &ssl_key, &ssl_key_p,
        &local_infile, &local_infile_p, &charset, &charset_p, &reconnect_p,
        &connect_timeout, &connect_timeout_p,
        &read_timeout, &read_timeout_p,
        &write_timeout, &write_timeout_p,
        &ssl_mode, &ssl_mode_p);
    conn_ = mysql_init(NULL);
    if (conn_ == NULL)
    {
        throw soci_error("mysql_init() failed.");
    }
    if (reconnect_p)
    {
        #if MYSQL_VERSION_ID < 8
            my_bool reconnect = 1;
        #else
            bool reconnect = 1;
        #endif
        if (0 != mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_RECONNECT) failed.");
        }
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
        if (0 != mysql_options(conn_, MYSQL_OPT_SSL_CA, ssl_ca.c_str()))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_SSL_CA) failed.");
        }
    }
    if (ssl_key_p)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_SSL_KEY, ssl_key.c_str()))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_SSL_KEY) failed.");
        }
    }
    if (ssl_cert_p)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_SSL_CERT, ssl_cert.c_str()))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_SSL_CERT) failed.");
        }
    }
    if (local_infile_p && local_infile == 1)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_LOCAL_INFILE, NULL))
        {
            clean_up();
            throw soci_error(
                "mysql_options() failed when trying to set local-infile.");
        }
    }
    if (connect_timeout_p)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_CONNECT_TIMEOUT) failed.");
        }
    }
    if (read_timeout_p)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_READ_TIMEOUT, &read_timeout))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_READ_TIMEOUT) failed.");
        }
    }
    if (write_timeout_p)
    {
        if (0 != mysql_options(conn_, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout))
        {
            clean_up();
            throw soci_error("mysql_options(MYSQL_OPT_WRITE_TIMEOUT) failed.");
        }
    }
#ifdef SOCI_HAS_MYSQL_SSL_OPT
    if (ssl_mode_p)
    {
    	if (0 != mysql_options(conn_, MYSQL_OPT_SSL_MODE, &ssl_mode))
    	{
       		clean_up();
       		throw soci_error("mysql_options(MYSQL_OPT_SSL_MODE) failed.");
       	}
    }
#endif
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

#if defined(__GNUC__) && ( __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 6)))
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif



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
        //throw soci_error(mysql_error(conn));
        string errMsg = mysql_error(conn);
        unsigned int errNum = mysql_errno(conn);
        throw mysql_soci_error(errMsg, errNum);

    }
}

} // namespace unnamed

bool mysql_session_backend::is_connected()
{
    return mysql_ping(conn_) == 0;
}

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
    session & /* s */, std::string const & /* table */, long long & value)
{
    value = static_cast<long long>(mysql_insert_id(conn_));

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
