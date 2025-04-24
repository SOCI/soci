//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci/firebird/soci-firebird.h"
#include "firebird/error-firebird.h"

#include <cstdlib>
#include <string>

namespace soci
{

firebird_soci_error::firebird_soci_error(std::string const & msg, ISC_STATUS const * status)
    : soci_error(msg)
{
    if (status != 0)
    {
        std::size_t i = 0;
        while (i < ISC_STATUS_LENGTH && status[i] != isc_arg_end)
        {
            status_.push_back(status[i++]);
        }
    }
}

int firebird_soci_error::get_backend_error_code() const
{
    // Search for the InterBase error code in the status vector, which consists
    // of clusters of 2 (mostly) or 3 (exceptionally, see below) elements.
    for (std::size_t i = 0; i < status_.size(); i += 2)
    {
        switch (status_[i])
        {
            case isc_arg_end:
                // This is never supposed to happen due to the way status_ is
                // filled in the ctor above.
                return 0;

            case isc_arg_gds:
                // Cluster starting with this value contains the error code
                // we're looking for in the next element.
                if (i + 1 < status_.size())
                {
                    return static_cast<int>(status_[i + 1]);
                }
                break;

            case isc_arg_cstring:
                // This is the only cluster consisting of 3 elements, so skip
                // an extra one.
                ++i;
                break;
        }
    }

    return 0;
}

soci_error::error_category firebird_soci_error::get_error_category() const
{
    switch (get_backend_error_code())
    {
        case isc_bad_db_format:
        case isc_unavailable:
        case isc_wrong_ods:
        case isc_badodsver:
        case isc_connect_reject:
        case isc_login:
        case isc_network_error:
        case isc_net_connect_err:
        case isc_lost_db_connection:
            return connection_error;

        case isc_syntaxerr:
        case isc_dsql_error:
        case isc_command_end_err:
            return invalid_statement;

        case isc_no_priv:
            return no_privilege;

        case isc_not_valid:
        case isc_no_dup:
        case isc_foreign_key:
        case isc_primary_key_ref:
        case isc_primary_key_notnull:
            return constraint_violation;

        case isc_tra_state:
            return unknown_transaction_state;

        case isc_io_error:
        case isc_virmemexh:
            return system_error;
    }

    return unknown;
}

namespace details
{

namespace firebird
{

void get_iscerror_details(ISC_STATUS * status_vector, std::string &msg)
{
    // Size of buffer for error messages: 4K should be enough for everybody.
    constexpr std::size_t const SOCI_FIREBIRD_ERRMSG = 4096;

    char msg_buffer[SOCI_FIREBIRD_ERRMSG];
    const ISC_STATUS *pvector = status_vector;

    try
    {
        // fetching first error message
        fb_interpret(msg_buffer, SOCI_FIREBIRD_ERRMSG, &pvector);
        msg = msg_buffer;

        // fetching next errors
        while (fb_interpret(msg_buffer, SOCI_FIREBIRD_ERRMSG, &pvector))
        {
            msg += "\n";
            msg += msg_buffer;
        }
    }
    catch (...)
    {
        throw firebird_soci_error("Exception caught while fetching error information");
    }
}

bool check_iscerror(ISC_STATUS const * status_vector, long errNum)
{
    std::size_t i=0;
    while (status_vector[i] != 0)
    {
        if (status_vector[i] == 1 && status_vector[i+1] == errNum)
        {
            return true;
        }
        ++i;
    }

    return false;
}
void throw_iscerror(ISC_STATUS * status_vector)
{
    std::string msg;

    get_iscerror_details(status_vector, msg);
    throw firebird_soci_error(msg, status_vector);
}

} // namespace firebird

} // namespace details

} // namespace soci
