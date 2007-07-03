//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
// retrieves service name, user name and password from the
// uniform connect string
void chop_connect_string(std::string const &connectString,
    std::string &serviceName, std::string &userName, std::string &password)
{
    std::string tmp;
    for (std::string::const_iterator i = connectString.begin(),
             end = connectString.end(); i != end; ++i)
    {
        if (*i == '=')
        {
            tmp += ' ';
        }
        else
        {
            tmp += *i;
        }
    }

    serviceName.clear();
    userName.clear();
    password.clear();

    std::istringstream iss(tmp);
    std::string key, value;
    while (iss >> key >> value)
    {
        if (key == "service")
        {
            serviceName = value;
        }
        else if (key == "user")
        {
            userName = value;
        }
        else if (key == "password")
        {
            password = value;
        }
    }
}

// concrete factory for Empty concrete strategies
oracle_session_backend * oracle_backend_factory::make_session(
     std::string const &connectString) const
{
    std::string serviceName, userName, password;
    chop_connect_string(connectString, serviceName, userName, password);
    return new oracle_session_backend(serviceName, userName, password);
}

oracle_backend_factory const soci::oracle;
