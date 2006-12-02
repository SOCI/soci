//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include <soci.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef SOCI_PGSQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_PGSQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

OracleRowIDBackEnd::OracleRowIDBackEnd(OracleSessionBackEnd &session)
{
    sword res = OCIDescriptorAlloc(session.envhp_,
        reinterpret_cast<dvoid**>(&rowidp_), OCI_DTYPE_ROWID, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate the ROWID descriptor");
    }
}

OracleRowIDBackEnd::~OracleRowIDBackEnd()
{
    OCIDescriptorFree(rowidp_, OCI_DTYPE_ROWID);
}

