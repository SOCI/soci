//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/firebird/soci-firebird.h"
#include "firebird/error-firebird.h"
#include "soci/session.h"
#include <exception>
#include <map>
#include <string>

using namespace soci;
using namespace soci::details::firebird;

namespace
{

void setDPBOption(std::string& dpb, int const option, std::string const & value)
{

    if (dpb.empty())
    {
        dpb.append(1, static_cast<char>(isc_dpb_version1));
    }

    // now we are adding new option
    dpb.append(1, static_cast<char>(option));
    dpb.append(1, static_cast<char>(value.size()));
    dpb.append(value);
}

} // namespace anonymous

firebird_session_backend::firebird_session_backend(
    connection_parameters const & parameters) : dbhp_(0), trhp_(0)
                                         , decimals_as_strings_(false)
{
    auto params = parameters;
    params.extract_options_from_space_separated_string();

    ISC_STATUS stat[ISC_STATUS_LENGTH];
    std::string param;

    // preparing connection options
    std::string dpb;
    if (params.get_option("user", param))
    {
        setDPBOption(dpb, isc_dpb_user_name, param);
    }

    if (params.get_option("password", param))
    {
        setDPBOption(dpb, isc_dpb_password, param);
    }

    if (params.get_option("role", param))
    {
        setDPBOption(dpb, isc_dpb_sql_role_name, param);
    }

    if (params.get_option("charset", param))
    {
        setDPBOption(dpb, isc_dpb_lc_ctype, param);
    }

    if (!params.get_option("service", param))
    {
        throw soci_error("Service name not specified.");
    }

    // connecting data base
    if (isc_attach_database(stat, static_cast<short>(param.size()),
        const_cast<char*>(param.c_str()), &dbhp_,
        static_cast<short>(dpb.size()), const_cast<char*>(dpb.c_str())))
    {
        throw_iscerror(stat);
    }

    if (params.get_option("decimals_as_strings", param))
    {
        decimals_as_strings_ = param == "1" || param == "Y" || param == "y";
    }
}


void firebird_session_backend::start_transaction()
{
    if (trhp_ == 0)
    {
        ISC_STATUS stat[ISC_STATUS_LENGTH];
        if (isc_start_transaction(stat, &trhp_, 1, &dbhp_, 0, nullptr))
        {
            throw_iscerror(stat);
        }
    }
}

void firebird_session_backend::begin()
{
    // If an explicit transaction is already active, this function doesn't do
    // anything, nested transactions are not supported.
    if (explicitTransaction_)
        return;

    // If an implicit transaction is active, commit it first: the changes done
    // in it would have been already committed by the other backends using
    // auto-commit mode, so they shouldn't become part of the new transaction
    // and be rolled back if it is.
    commit();

    start_transaction();

    explicitTransaction_ = true;
}

firebird_session_backend::~firebird_session_backend() noexcept(false)
{
    cleanUp();
}

bool firebird_session_backend::is_connected()
{
    ISC_STATUS stat[ISC_STATUS_LENGTH];
    ISC_SCHAR req[] = { isc_info_ods_version, isc_info_end };
    ISC_SCHAR res[256];

    return isc_database_info(stat, &dbhp_, sizeof(req), req, sizeof(res), res) == 0;
}

void firebird_session_backend::commit()
{
    ISC_STATUS stat[ISC_STATUS_LENGTH];

    if (trhp_ != 0)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            throw_iscerror(stat);
        }

        trhp_ = 0;
        explicitTransaction_ = false;
    }
}

void firebird_session_backend::rollback()
{
    ISC_STATUS stat[ISC_STATUS_LENGTH];

    if (trhp_ != 0)
    {
        if (isc_rollback_transaction(stat, &trhp_))
        {
            throw_iscerror(stat);
        }

        trhp_ = 0;
        explicitTransaction_ = false;
    }
}

isc_tr_handle* firebird_session_backend::current_transaction()
{
    // It will do nothing if we're already inside a transaction.
    start_transaction();

    return &trhp_;
}

void firebird_session_backend::cleanUp()
{
    ISC_STATUS stat[ISC_STATUS_LENGTH];

    // Remember the first error, if any, to throw it only after doing as much
    // of the cleanup as possible.
    std::exception_ptr error;
    auto const remember_error = [&stat, &error]()
    {
        if (!error)
        {
            try
            {
                throw_iscerror(stat);
            }
            catch (...)
            {
                error = std::current_exception();
            }
        }
    };

    // Implicitly started transaction is committed, as there is no autocommit
    // mode in Firebird and so this is the only way to ensure that the changes
    // done outside of an explicit transaction are not lost, as with the other
    // backends.
    if (trhp_ != 0 && !explicitTransaction_)
    {
        if (isc_commit_transaction(stat, &trhp_))
        {
            // We need to roll it back, as the database can't be detached while
            // the transaction is still active.
            remember_error();
        }
    }

    // But an explicitly started and not committed transaction is rolled back,
    // as with the other backends (this also takes care of implicit transaction
    // that we failed to commit above).
    if (trhp_ != 0)
    {
        if (isc_rollback_transaction(stat, &trhp_))
            remember_error();

        trhp_ = 0;
    }

    if (isc_detach_database(stat, &dbhp_))
    {
        remember_error();
    }

    dbhp_ = 0L;
    explicitTransaction_ = false;

    if (error)
    {
        std::rethrow_exception(error);
    }
}

bool firebird_session_backend::get_next_sequence_value(
    session & s, std::string const & sequence, long long & value)
{
    // We could use isq_execute2() directly but this is even simpler.
    s << "select next value for " + sequence + " from rdb$database",
          into(value);

    return true;
}

firebird_statement_backend * firebird_session_backend::make_statement_backend()
{
    return new firebird_statement_backend(*this);
}

details::rowid_backend* firebird_session_backend::make_rowid_backend()
{
    throw soci_error("RowIDs are not supported");
}

firebird_blob_backend * firebird_session_backend::make_blob_backend()
{
    return new firebird_blob_backend(*this);
}
