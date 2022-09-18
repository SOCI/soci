//
// Copyright (C) 2020 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_AUTOSTATEMENT_H_INCLUDED
#define SOCI_PRIVATE_SOCI_AUTOSTATEMENT_H_INCLUDED

namespace soci
{

namespace details
{

// This helper class can be used with any statement backend to initialize and
// cleanup a statement backend object in a RAII way. Normally this is not
// needed because it's done by statement_impl, but this can be handy when using
// a concrete backend inside this backend own code, see e.g. ODBC session
// implementation.
template <typename Backend>
struct auto_statement : Backend
{
    template <typename Session>
    explicit auto_statement(Session& session)
        : Backend(session)
    {
        this->alloc();
    }

    ~auto_statement() override
    {
        this->clean_up();
    }
};

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_AUTOSTATEMENT_H_INCLUDED
