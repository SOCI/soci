//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "error-firebird.h"
#include <map>
#include <sstream>
#include <string>

using namespace soci;
using namespace soci::details::firebird;

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

firebird_session_backend::firebird_session_backend(
    std::string const & connectString) : dbhp_(0), trhp_(0)
{
    // extract connection parameters
    std::map<std::string, std::string> params;
    explodeISCConnectString(connectString, params);

    ISC_STATUS stat[stat_size];
    std::string param;

    // preparing connection options
    if (getISCConnectParameter(params, "user", param))
    {
        setDPBOption(isc_dpb_user_name, param);
    }

    if (getISCConnectParameter(params, "password", param))
    {
        setDPBOption(isc_dpb_password, param);
    }

    if (getISCConnectParameter(params, "role", param))
    {
        setDPBOption(isc_dpb_sql_role_name, param);
    }

    if (getISCConnectParameter(params, "charset", param))
    {
        setDPBOption(isc_dpb_lc_ctype, param);
    }

    if (getISCConnectParameter(params, "service", param) == false)
    {
        throw soci_error("Service name not specified.");
    }

    // connecting data base
    if (isc_attach_database(stat, static_cast<short>(param.size()),
        const_cast<char*>(param.c_str()), &dbhp_,
        static_cast<short>(dpb_.size()), const_cast<char*>(dpb_.c_str())))
    {
        throw_iscerror(stat);
    }

    // starting transaction
    begin();
}


void firebird_session_backend::begin()
{
    // Transaction is always started in ctor, because Firebird can't work
    // without active transaction.
    // Transaction will be automatically commited in cleanUp method.
    if (trhp_ == 0)
    {
        ISC_STATUS stat[stat_size];
        if (isc_start_transaction(stat, &trhp_, 1, &dbhp_, 0, NULL))
        {
            throw_iscerror(stat);
        }
    }
}

firebird_session_backend::~firebird_session_backend()
{
    cleanUp();
}

void firebird_session_backend::setDPBOption(int const option, std::string const & value)
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

void firebird_session_backend::commit()
{
    ISC_STATUS stat[stat_size];

    if (trhp_ != 0)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            throw_iscerror(stat);
        }

        trhp_ = 0;
    }

#ifndef SOCI_FIREBIRD_NORESTARTTRANSACTION
    begin();
#endif

}

void firebird_session_backend::rollback()
{
    ISC_STATUS stat[stat_size];

    if (trhp_ != 0)
    {
        if (isc_rollback_transaction(stat, &trhp_))
        {
            throw_iscerror(stat);
        }

        trhp_ = 0;
    }

#ifndef SOCI_FIREBIRD_NORESTARTTRANSACTION
    begin();
#endif

}

void firebird_session_backend::cleanUp()
{
    ISC_STATUS stat[stat_size];

    // at the end of session our transaction is finally commited.
    if (trhp_ != 0)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            throw_iscerror(stat);
        }

        trhp_ = 0;
    }

    if (isc_detach_database(stat, &dbhp_))
    {
        throw_iscerror(stat);
    }

    dbhp_ = 0L;
}

firebird_statement_backend * firebird_session_backend::make_statement_backend()
{
    return new firebird_statement_backend(*this);
}

firebird_rowid_backend * firebird_session_backend::make_rowid_backend()
{
    return new firebird_rowid_backend(*this);
}

firebird_blob_backend * firebird_session_backend::make_blob_backend()
{
    return new firebird_blob_backend(*this);
}
