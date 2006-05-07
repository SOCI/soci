//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-oracle.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
// retrieves service name, user name and password from the
// uniform connect string
void chopConnectString(std::string const &connectString,
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
OracleSessionBackEnd * OracleBackEndFactory::makeSession(
     std::string const &connectString) const
{
    std::string serviceName, userName, password;
    chopConnectString(connectString, serviceName, userName, password);
    return new OracleSessionBackEnd(serviceName, userName, password);
}

OracleBackEndFactory const SOCI::oracle;
