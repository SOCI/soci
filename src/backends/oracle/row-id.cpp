//
// Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/oracle/soci-oracle.h"

using namespace soci;
using namespace soci::details;

oracle_rowid_backend::oracle_rowid_backend(oracle_session_backend &session)
{
    sword res = OCIDescriptorAlloc(session.envhp_,
        reinterpret_cast<dvoid**>(&rowidp_), OCI_DTYPE_ROWID, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw soci_error("Cannot allocate the ROWID descriptor");
    }
}

oracle_rowid_backend::~oracle_rowid_backend()
{
    OCIDescriptorFree(rowidp_, OCI_DTYPE_ROWID);
}
