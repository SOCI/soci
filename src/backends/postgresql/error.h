//
// Copyright (C) 2011 Gevorg Voskanyan
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_POSTGRESQL_ERROR_H_INCLUDED
#define SOCI_POSTGRESQL_ERROR_H_INCLUDED

#include "soci-postgresql.h"

namespace soci
{

namespace details
{

namespace postgresql
{

void throw_postgresql_soci_error(PGresult*& res);

void get_error_details(PGresult* res, std::string &msg, std::string &sqlstate);

} // namespace postgresql

} // namespace details

} // namespace soci

#endif
