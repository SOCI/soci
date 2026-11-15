//
// Copyright (C) 2026 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_REPORT_ERROR_H_INCLUDED
#define SOCI_PRIVATE_SOCI_REPORT_ERROR_H_INCLUDED

#include "soci/session.h"

namespace soci
{

namespace details
{

// Report the exception currently being handled using the logger of the given
// session after adding the specified context to it.
//
// This function must be called from a catch clause and never throws.
void report_current_exception(session const & sql,
                              char const * context) noexcept;

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_REPORT_ERROR_H_INCLUDED
