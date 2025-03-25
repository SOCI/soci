//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"

using namespace soci;
using namespace soci::details;


postgresql_rowid_backend::postgresql_rowid_backend(
    postgresql_session_backend & /* session */)
    : value_(0)
{
}

postgresql_rowid_backend::~postgresql_rowid_backend()
{
    // nothing to do here
}
