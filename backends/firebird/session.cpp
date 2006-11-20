//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "error.h"
#include <map>
#include <sstream>
#include <string>

using namespace SOCI;
using namespace SOCI::details::Firebird;

namespace
{
// retrieves parameters from the uniform connect string
    void explodeISCConnectString(std::string const &connectString,
                                 std::map<std::string, std::string> &parameters)
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

        parameters.clear();

        std::istringstream iss(tmp);
        std::string key, value;
        while (iss >> key >> value)
        {
            parameters.insert(std::pair<std::string, std::string>(key, value));
        }
    }

// extracts given parameter from map previusly build with explodeISCConnectString
    bool getISCConnectParameter(std::map<std::string, std::string> const & m, std::string const & key,
                                std::string & value)
    {
        std::map <std::string, std::string> :: const_iterator i;
        value.clear();

        i = m.find(key);

        if (i != m.end())
        {
            value = i->second;
            return true;
        }
        else
        {
            return false;
        }
    }

} // namespace anonymous

FirebirdSessionBackEnd::FirebirdSessionBackEnd(
    std::string const & connectString) : dbhp_(0), trhp_(0)
{
    // extract connection parameters
    std::map<std::string, std::string> params;
    explodeISCConnectString(connectString, params);

    ISC_STATUS stat[stat_size];
    std::string param;

    // preparing connection options
    if (getISCConnectParameter(params, "user", param))
        setDPBOption(isc_dpb_user_name, param);

    if (getISCConnectParameter(params, "password", param))
        setDPBOption(isc_dpb_password, param);

    if (getISCConnectParameter(params, "role", param))
        setDPBOption(isc_dpb_sql_role_name, param);

    if (getISCConnectParameter(params, "charset", param))
        setDPBOption(isc_dpb_lc_ctype, param);

    if (!getISCConnectParameter(params, "service", param))
        throw SOCIError("Service name not specified.");

    // connecting data base
    if (isc_attach_database(stat, static_cast<short>(param.size()),
                            const_cast<char*>(param.c_str()), &dbhp_,
                            static_cast<short>(dpb_.size()), const_cast<char*>(dpb_.c_str())))
    {
        throwISCError(stat);
    }

    // starting transaction
    begin();
}


void FirebirdSessionBackEnd::begin()
{
    // Transaction is always started in ctor, because Firebird can't work
    // without active transaction.
    // Transaction will be automatically commited in cleanUp method.
    if (trhp_ == 0)
    {
        ISC_STATUS stat[stat_size];
        if (isc_start_transaction(stat, &trhp_, 1, &dbhp_, 0, NULL))
        {
            throwISCError(stat);
        }
    }
}

FirebirdSessionBackEnd::~FirebirdSessionBackEnd()
{
    cleanUp();
}

void FirebirdSessionBackEnd::setDPBOption(int const option, std::string const & value)
{

    if (dpb_.size() == 0)
    {
        dpb_.append(1, static_cast<char>(isc_dpb_version1));
    }

    // now we are adding new option
    dpb_.append(1, static_cast<char>(option));
    dpb_.append(1, static_cast<char>(value.size()));
    dpb_.append(value);
}

void FirebirdSessionBackEnd::commit()
{
    ISC_STATUS stat[stat_size];

    if (trhp_ != 0)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            throwISCError(stat);
        }

        trhp_ = 0;
    }

#ifndef SOCI_FIREBIRD_NORESTARTTRANSACTION
    begin();
#endif

}

void FirebirdSessionBackEnd::rollback()
{
    ISC_STATUS stat[stat_size];

    if (trhp_ != 0)
    {
        if (isc_rollback_transaction(stat, &trhp_))
        {
            throwISCError(stat);
        }

        trhp_ = 0;
    }

#ifndef SOCI_FIREBIRD_NORESTARTTRANSACTION
    begin();
#endif

}

void FirebirdSessionBackEnd::cleanUp()
{
    ISC_STATUS stat[stat_size];

    // at the end of session our transaction is finally commited.
    if (trhp_ != 0)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            throwISCError(stat);
        }

        trhp_ = 0;
    }

    if (isc_detach_database(stat, &dbhp_))
    {
        throwISCError(stat);
    }

    dbhp_ = 0L;
}

FirebirdStatementBackEnd * FirebirdSessionBackEnd::makeStatementBackEnd()
{
    return new FirebirdStatementBackEnd(*this);
}

FirebirdRowIDBackEnd * FirebirdSessionBackEnd::makeRowIDBackEnd()
{
    return new FirebirdRowIDBackEnd(*this);
}

FirebirdBLOBBackEnd * FirebirdSessionBackEnd::makeBLOBBackEnd()
{
    return new FirebirdBLOBBackEnd(*this);
}


