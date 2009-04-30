//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"

using namespace soci;

firebird_session_backend * firebird_backend_factory::make_session(
    std::string const &connectString) const
{
    return new firebird_session_backend(connectString);
}

firebird_backend_factory const soci::firebird;

extern "C"
{

// for dynamic backend loading
SOCI_FIREBIRD_DECL backend_factory const * factory_firebird()
{

    return &soci::firebird;
}

} // extern "C"
