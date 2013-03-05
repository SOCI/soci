//
// Copyright (C) 2011 Gevorg Voskanyan
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include "error.h"
#include <cstring>
#include <cassert>

using namespace soci;
using namespace soci::details;
using namespace soci::details::postgresql;

postgresql_soci_error::postgresql_soci_error(
    std::string const & msg, char const *sqlst)
    : soci_error(msg)
{
    assert(std::strlen(sqlst) == 5);
    std::memcpy(sqlstate_, sqlst, 5);
}

std::string postgresql_soci_error::sqlstate() const
{
    return std::string(sqlstate_, 5);
}

void soci::details::postgresql::get_error_details(PGresult *result,
    std::string &msg, std::string &sqlstate)
{
    assert(result);

    msg = PQresultErrorMessage(result);
    const char *sqlst = PQresultErrorField(result, PG_DIAG_SQLSTATE);
    const char* blank_sql_state = "     ";
    if (!sqlst)
    {
        sqlst = blank_sql_state;
    }
    assert(sqlst && std::strlen(sqlst) == 5);
    sqlstate.assign(sqlst, 5);
}

void soci::details::postgresql::throw_postgresql_soci_error(PGresult*& result)
{
    std::string msg;
    std::string sqlstate;

    get_error_details(result, msg, sqlstate);
    PQclear(result);
    result = NULL;
    throw postgresql_soci_error(msg, sqlstate.c_str());
}
