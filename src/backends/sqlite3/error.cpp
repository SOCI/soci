//
// Copyright 2014 SimpliVT Corporation
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"
#include <cstring>

using namespace soci;

namespace
{

// Combine the possibly empty prefix and the message.
std::string combine(std::string const& prefix, char const* message)
{
    std::string full{prefix};

    if (!full.empty())
      full += ": ";

    full += message;

    return full;
}

} // anonymous namespace

sqlite3_soci_error::sqlite3_soci_error(
    sqlite_api::sqlite3* conn,
    std::string const & prefix,
    char const* errmsg)
    : soci_error(combine(prefix, errmsg ? errmsg : sqlite3_errmsg(conn))),
      extended_result_(sqlite3_extended_errcode(conn))
{
}

int sqlite3_soci_error::result() const
{
    // Primary result is contained in the least significant 8 bits of the
    // SQLite extended result.
    return extended_result_ & 0xff;
}

int sqlite3_soci_error::extended_result() const
{
    return extended_result_;
}
