//
// Copyright (C) 2011 Gevorg Voskanyan
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"
#include "soci/callbacks.h"
#include "soci/connection-parameters.h"

#include <libpq-fe.h>

#include <cstring>

using namespace soci;
using namespace soci::details;

postgresql_soci_error::postgresql_soci_error(
    std::string const & msg, char const *sqlst)
    : soci_error(msg)
{
    std::memcpy(sqlstate_, sqlst, 5);
}

soci_error::error_category postgresql_soci_error::get_error_category() const
{
    if (std::memcmp(sqlstate_, "08", 2) == 0)
    {
        return connection_error;
    }
    else if (std::memcmp(sqlstate_, "42501", 5) == 0)
    {
        return no_privilege;
    }
    else if (std::memcmp(sqlstate_, "42", 2) == 0)
    {
        return invalid_statement;
    }
    else if (std::memcmp(sqlstate_, "02", 2) == 0)
    {
        return no_data;
    }
    else if (std::memcmp(sqlstate_, "23", 2) == 0)
    {
        return constraint_violation;
    }
    else if ((std::memcmp(sqlstate_, "53", 2) == 0) ||
        (std::memcmp(sqlstate_, "54", 2) == 0) ||
        (std::memcmp(sqlstate_, "58", 2) == 0) ||
        (std::memcmp(sqlstate_, "XX", 2) == 0))
    {
        return system_error;
    }

    return unknown;
}

std::string postgresql_soci_error::get_sqlstate() const
{
    return std::string(sqlstate_, 5);
}

void
details::postgresql_result::check_for_errors(char const* errMsg) const
{
    static_cast<void>(check_for_data(errMsg));
}

bool
details::postgresql_result::check_for_data(char const* errMsg) const
{
    // This SQL state will be used if we can't get anything more precise.
    const char* fallback_sql_state = "     ";

    std::string msg(errMsg);

    ExecStatusType const status = PQresultStatus(result_);
    switch (status)
    {
        case PGRES_EMPTY_QUERY:
        case PGRES_COMMAND_OK:
            // No data but don't throw neither.
            return false;

        case PGRES_TUPLES_OK:
            return true;

        case PGRES_FATAL_ERROR:
            msg += " Fatal error.";

            if (PQstatus(sessionBackend_.conn_) == CONNECTION_BAD)
            {
                msg += " Connection failed.";

                // It's useful to set it here to something at least slightly
                // more specific, as we're not going to get anything from
                // PG_DIAG_SQLSTATE below if the connection is lost.
                fallback_sql_state = "08000"; // connection_exception

                // call the failover callback, if registered

                failover_callback * callback = sessionBackend_.failoverCallback_;
                if (callback != NULL)
                {
                    bool reconnected = false;

                    try
                    {
                        callback->started();
                    }
                    catch (...)
                    {
                        // ignore exceptions from user callbacks
                    }
                    bool retry = false;
                    do {
                        std::string newTarget;

                        try
                        {
                            callback->failed(retry, newTarget);
                        }
                        catch (...)
                        {
                            // do not continue execution because
                            // user callback generated an exception
                            retry = false;
                        }

                        if (retry)
                        {
                            connection_parameters parameters =
                                sessionBackend_.connectionParameters_;

                            if (!newTarget.empty())
                                parameters.set_connect_string(newTarget);

                            sessionBackend_.clean_up();

                            sessionBackend_.connect(parameters);

                            reconnected = true;
                        }
                    } while (retry && !reconnected);

                    if (reconnected == false)
                    {
                        try
                        {
                            callback->aborted();
                        }
                        catch (...)
                        {
                            // ignore exceptions from user callbacks
                        }
                    }
                    else
                    {
                        try
                        {
                            callback->finished(*sessionBackend_.session_);
                        }
                        catch (...)
                        {
                            // ignore exceptions from user callbacks
                        }
                    }
                }
            }

            break;

        default:
            // Some of the other status codes are not really errors but we're
            // not prepared to handle them right now and shouldn't ever receive
            // them so throw nevertheless

            break;
    }

    const char* const pqError = PQresultErrorMessage(result_);
    if (pqError && *pqError)
    {
        msg += " ";
        msg += pqError;
    }

    const char* sqlstate = PQresultErrorField(result_, PG_DIAG_SQLSTATE);
    if (!sqlstate)
    {
        sqlstate = fallback_sql_state;
    }

    throw postgresql_soci_error(msg, sqlstate);
}
