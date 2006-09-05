//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-mysql.h"
#include <ciso646>
#include <cerrno>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using std::string;


namespace { // anonymous

void skipWhite(string::const_iterator *i,
    string::const_iterator const & end, bool endok)
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
                throw SOCIError("Unexpected end of connection string.");
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

string paramName(string::const_iterator *i,
    string::const_iterator const & end)
{
    string val("");
    for (;;)
    {
        if (*i == end or (!std::isalpha(**i) and **i != '_'))
        {
            break;
        }
        val += **i;
        ++*i;
    }
    return val;
}

string paramValue(string::const_iterator *i,
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
                throw SOCIError(err);
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
                throw SOCIError(err);
            }
        }
        if (!quot and std::isspace(**i))
        {
            break;
        }
        if (**i == '\\')
        {
            ++*i;
            if (*i == end)
            {
                throw SOCIError(err);
            }
        }
        val += **i;
        ++*i;
    }
    return val;
}

bool validInt(const string & s)
{
    char *tail;
    const char *cstr = s.c_str();
    errno = 0;
    long l = std::strtol(cstr, &tail, 10);
    if (errno != 0 or l > INT_MAX or l < INT_MIN)
    {
        return false;
    }
    if (*tail != '\0')
    {
        return false;
    }
    return true;
}

void parseConnectString(const string & connectString,
    string *host, bool *host_p,
    string *user, bool *user_p,
    string *password, bool *password_p,
    string *db, bool *db_p,
    string *unix_socket, bool *unix_socket_p,
    int *port, bool *port_p)
{
    *host_p = false;
    *user_p = false;
    *password_p = false;
    *db_p = false;
    *unix_socket_p = false;
    *port_p = false;
    string err = "Malformed connection string.";
    string::const_iterator i = connectString.begin(),
        end = connectString.end();
    while (i != end)
    {
        skipWhite(&i, end, true);
        if (i == end)
        {
            return;
        }
        string par = paramName(&i, end);
        skipWhite(&i, end, false);
        if (*i == '=')
        {
            ++i;
        }
        else
        {
            throw SOCIError(err);
        }
        skipWhite(&i, end, false);
        string val = paramValue(&i, end);
        if (par == "port" and !*port_p)
        {
            if (!validInt(val))
            {
                throw SOCIError(err);
            }
            *port = std::atoi(val.c_str());
            if (port < 0)
            {
                throw SOCIError(err);
            }
            *port_p = true;
        }
        else if (par == "host" and !*host_p)
        {
            *host = val;
            *host_p = true;
        }
        else if (par == "user" and !*user_p)
        {
            *user = val;
            *user_p = true;
        }
        else if ((par == "pass" or par == "password") and !*password_p)
        {
            *password = val;
            *password_p = true;
        }
        else if ((par == "db" or par == "dbname") and !*db_p)
        {
            *db = val;
            *db_p = true;
        }
        else if (par == "unix_socket" && !*unix_socket_p)
        {
            *unix_socket = val;
            *unix_socket_p = true;
        }
        else
        {
            throw SOCIError(err);
        }
    }
}

} // namespace anonymous

MySQLSessionBackEnd::MySQLSessionBackEnd(
    std::string const & connectString)
{
    string host, user, password, db, unix_socket;
    int port;
    bool host_p, user_p, password_p, db_p, unix_socket_p, port_p;
    parseConnectString(connectString, &host, &host_p, &user, &user_p,
        &password, &password_p, &db, &db_p,
        &unix_socket, &unix_socket_p, &port, &port_p);
    conn_ = mysql_init(NULL);
    if (conn_ == NULL)
    {
        throw SOCIError("mysql_init() failed.");
    }
    if (!mysql_real_connect(conn_,
            host_p ? host.c_str() : NULL,
            user_p ? user.c_str() : NULL,
            password_p ? password.c_str() : NULL,
            db_p ? db.c_str() : NULL,
            port_p ? port : 0,
            unix_socket_p ? unix_socket.c_str() : NULL,
            0)) {
        string err = mysql_error(conn_);
        cleanUp();
        throw SOCIError(err);
    }
}

MySQLSessionBackEnd::~MySQLSessionBackEnd()
{
    cleanUp();
}

namespace { // anonymous

// helper function for hardcoded queries
void hardExec(MYSQL *conn, const string & query)
{
    //cerr << query << endl;
    if (0 != mysql_real_query(conn, query.c_str(), query.size()))
    {
        throw SOCIError(mysql_error(conn));
    }
}

}  // namespace anonymous

void MySQLSessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN");
}

void MySQLSessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT");
}

void MySQLSessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK");
}

void MySQLSessionBackEnd::cleanUp()
{
    if (conn_ != NULL)
    {
        mysql_close(conn_);
        conn_ = NULL;
    }
}

MySQLStatementBackEnd * MySQLSessionBackEnd::makeStatementBackEnd()
{
    return new MySQLStatementBackEnd(*this);
}

MySQLRowIDBackEnd * MySQLSessionBackEnd::makeRowIDBackEnd()
{
    return new MySQLRowIDBackEnd(*this);
}

MySQLBLOBBackEnd * MySQLSessionBackEnd::makeBLOBBackEnd()
{
    return new MySQLBLOBBackEnd(*this);
}

