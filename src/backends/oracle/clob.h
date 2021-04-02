//
// Copyright (C) 2021 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ORACLE_CLOB_H_INCLUDED
#define SOCI_ORACLE_CLOB_H_INCLUDED

#include "soci/oracle/soci-oracle.h"

// Note that mopst functions in this file are currently implemented in
// standard-use-type.cpp except for read_from_lob() which is found in
// standard-into-type.cpp.

namespace soci
{

namespace details
{

namespace oracle
{

// Creates and returns a temporary LOB object. Throws on error.
OCILobLocator * create_temp_lob(oracle_session_backend& session);

// Writes the given value to the LOB. Throws on error.
void write_to_lob(oracle_session_backend& session,
    OCILobLocator * lobp, const std::string & value);

// Reads the value from the lob into the given string. Throws on error.
void read_from_lob(oracle_session_backend& session,
    OCILobLocator * lobp, std::string & value);

// Frees a temporary LOB object. Doesn't throw.
void free_temp_lob(oracle_session_backend& session, OCILobLocator * lobp);

} // namespace oracle

} // namespace details

} // namespace soci

#endif // SOCI_ORACLE_CLOB_H_INCLUDED
