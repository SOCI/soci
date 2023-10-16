//
// Copyright (C) 2021 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ORACLE_DATETIME_H_INCLUDED
#define SOCI_ORACLE_DATETIME_H_INCLUDED

#include "soci/type-wrappers.h"
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

// Writes the given value to the OCIDateTime object. Throws on error.
void write_to_oci_datetime(oracle_session_backend& session, OCIDateTime * dtm, const soci::datetime & value);

// Reads the value from the OCIDateTime object. Throws on error.
void read_from_oci_datetime ( oracle_session_backend& session, OCIDateTime* dtm, soci::datetime& value );

// Allocate OCIDateTime object. Throws on error.
OCIDateTime* alloc_oci_datetime ( oracle_session_backend& session );

void free_oci_datetime ( OCIDateTime* dtm );


} // namespace oracle

} // namespace details

} // namespace soci

#endif // SOCI_ORACLE_DATETIME_H_INCLUDED
