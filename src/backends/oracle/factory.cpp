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
void chop_connect_string(std::string const & connectString,
    std::string & serviceName, std::string & userName,
    std::string & password, int & mode)
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
    mode = OCI_DEFAULT;

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
        else if (key == "mode")
        {
            if (value == "sysdba")
            {
                mode = OCI_SYSDBA;
            }
            else if (value == "sysoper")
            {
                mode = OCI_SYSOPER;
            }
            else if (value == "default")
            {
                mode = OCI_DEFAULT;
            }
            else
            {
                throw soci_error("Invalid connection mode.");
            }
        }
    }
}

// concrete factory for Empty concrete strategies
oracle_session_backend * oracle_backend_factory::make_session(
     std::string const &connectString) const
{
    std::string serviceName, userName, password;
    int mode;

    chop_connect_string(connectString, serviceName, userName, password, mode);

    return new oracle_session_backend(serviceName, userName, password, mode);
}

oracle_backend_factory const soci::oracle;

extern "C"
{

// for dynamic backend loading
SOCI_ORACLE_DECL backend_factory const * factory_oracle()
{
    return &soci::oracle;
}

} // extern "C"
