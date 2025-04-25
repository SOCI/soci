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

soci_error::error_category sqlite3_soci_error::get_error_category() const
{
  switch ( result() )
  {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
      // These are not errors at all and should never be used here.
      return soci_error::unknown;

    case SQLITE_ERROR:
    case SQLITE_INTERNAL:
    case SQLITE_ABORT:
    case SQLITE_BUSY:
    case SQLITE_LOCKED:
    case SQLITE_NOMEM:
    case SQLITE_INTERRUPT:
    case SQLITE_IOERR:
    case SQLITE_CORRUPT:
    case SQLITE_NOTFOUND:
    case SQLITE_FULL:
    case SQLITE_PROTOCOL:
    case SQLITE_SCHEMA:
    case SQLITE_NOLFS:
      return soci_error::system_error;

    case SQLITE_PERM:
    case SQLITE_READONLY:
    case SQLITE_AUTH:
      return soci_error::no_privilege;

    case SQLITE_CANTOPEN:
    case SQLITE_NOTADB:
      return soci_error::connection_error;

    case SQLITE_TOOBIG:
    case SQLITE_CONSTRAINT:
    case SQLITE_MISMATCH:
      return soci_error::constraint_violation;

    case SQLITE_MISUSE:
    case SQLITE_RANGE:
      return soci_error::invalid_statement;

    case SQLITE_EMPTY:
    case SQLITE_FORMAT:
    case SQLITE_NOTICE:
    case SQLITE_WARNING:
      // These result codes are either unused or not used as errors, according
      // to the SQLite documentation.
      return soci_error::unknown;
  }

  // Unknown SQLite error code?
  return soci_error::unknown;
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
