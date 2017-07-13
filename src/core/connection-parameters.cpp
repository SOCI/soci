//
// Copyright (C) 2013 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/connection-parameters.h"
#include "soci/soci-backend.h"
#include "soci/backend-loader.h"

using namespace soci;

namespace // anonymous
{

void parseConnectString(std::string const & connectString,
    std::string & backendName,
    std::string & connectionParameters)
{
    std::string const protocolSeparator = "://";

    std::string::size_type const p = connectString.find(protocolSeparator);
    if (p == std::string::npos)
    {
        throw soci_error("No backend name found in " + connectString);
    }

    backendName = connectString.substr(0, p);
    connectionParameters = connectString.substr(p + protocolSeparator.size());
}

} // namespace anonymous

connection_parameters::connection_parameters()
    : factory_(NULL)
{
}

connection_parameters::connection_parameters(backend_factory const & factory,
    std::string const & connectString)
    : factory_(&factory), connectString_(connectString)
{
    parse();
}

connection_parameters::connection_parameters(std::string const & backendName,
    std::string const & connectString)
    : factory_(&dynamic_backends::get(backendName)), connectString_(connectString)
{
    parse();
}

connection_parameters::connection_parameters(std::string const & fullConnectString)
{
    std::string backendName;
    std::string connectString;

    parseConnectString(fullConnectString, backendName, connectString);

    factory_ = &dynamic_backends::get(backendName);
    connectString_ = connectString;
    parse();
}

void connection_parameters::parse()
{
    std::stringstream ssconn(connectString_);
    while (!ssconn.eof() && ssconn.str().find('=') != std::string::npos)
    {
        std::string key, val;
        std::getline(ssconn, key, '=');
        std::getline(ssconn, val, ' ');

        char delim = 0;
        if (val.size()>0 && (val[0]=='\"' || val[0]=='\''))
        {
            delim = val[0];
            std::string quotedVal = val.erase(0, 1);

            if (quotedVal[quotedVal.size()-1] ==  delim)
            {
                quotedVal.erase(val.size()-1);
            }
            else // space inside value string
            {
                std::getline(ssconn, val, delim);
                quotedVal = quotedVal + " " + val;
                std::string keepspace;
                std::getline(ssconn, keepspace, ' ');
            }

            val = quotedVal;
        }
        set_option(key.c_str(), val);
    }
}
