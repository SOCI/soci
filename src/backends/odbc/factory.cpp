//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <soci.h>

using namespace SOCI;
using namespace SOCI::details;


// concrete factory for ODBC concrete strategies
ODBCSessionBackEnd * ODBCBackEndFactory::makeSession(
     std::string const &connectString) const
{
     return new ODBCSessionBackEnd(connectString);
}

ODBCBackEndFactory const SOCI::odbc;
