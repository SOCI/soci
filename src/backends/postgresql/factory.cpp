//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"
#include "soci/connection-parameters.h"
#include "soci/backend-loader.h"

using namespace soci;
using namespace soci::details;

postgresql_session_backend * postgresql_backend_factory::make_session(
     connection_parameters const & parameters) const
{
    return new postgresql_session_backend(parameters);
}

postgresql_backend_factory const soci::postgresql;

extern "C"
{

// for dynamic backend loading
SOCI_POSTGRESQL_DECL backend_factory const * factory_postgresql()
{
    return &soci::postgresql;
}

SOCI_POSTGRESQL_DECL void register_factory_postgresql()
{
    soci::dynamic_backends::register_backend("postgresql", soci::postgresql);
}

} // extern "C"
