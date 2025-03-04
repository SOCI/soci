//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci/oracle/soci-oracle.h"
#include "soci/connection-parameters.h"
#include "soci/backend-loader.h"

#include "soci-cstrtoi.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

// decode charset and ncharset names
int charset_code(const std::string & name)
{
    // Note: unofficial reference for charset ids is:
    // http://www.mydul.net/charsets.html

    int code;

    if (name == "utf8")
    {
        code = 871;
    }
    else if (name == "utf16")
    {
        code = OCI_UTF16ID;
    }
    else if (name == "we8mswin1252" || name == "win1252")
    {
        code = 178;
    }
    else
    {
        // allow explicit number value
        if (!cstring_to_unsigned(code, name.c_str()))
        {
            throw soci_error("Invalid character set name.");
        }
    }

    return code;
}

oracle_session_backend * oracle_backend_factory::make_session(
     connection_parameters const & parameters) const
{
    std::string value;

    auto params = parameters;
    params.extract_options_from_space_separated_string();

    std::string serviceName, userName, password;
    params.get_option("service", serviceName);
    params.get_option("user", userName);
    params.get_option("password", password);

    int mode = OCI_DEFAULT;
    if (params.get_option("mode", value))
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

    bool decimals_as_strings = false;
    if (params.get_option("decimals_as_strings", value))
    {
        decimals_as_strings = value == "1" || value == "Y" || value == "y";
    }

    int charset = 0;
    if (params.get_option("charset", value))
    {
        charset = charset_code(value);
    }

    int ncharset = 0;
    if (params.get_option("ncharset", value))
    {
        ncharset = charset_code(value);
    }

    return new oracle_session_backend(serviceName, userName, password,
        mode, decimals_as_strings, charset, ncharset);
}

oracle_backend_factory const soci::oracle;

extern "C"
{

// for dynamic backend loading
SOCI_ORACLE_DECL backend_factory const * factory_oracle()
{
    return &soci::oracle;
}

SOCI_ORACLE_DECL void register_factory_oracle()
{
    soci::dynamic_backends::register_backend("oracle", soci::oracle);
}

} // extern "C"
