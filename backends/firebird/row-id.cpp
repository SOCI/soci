//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"

using namespace SOCI;

FirebirdRowIDBackEnd::FirebirdRowIDBackEnd(FirebirdSessionBackEnd & /* session */)
{
    // Unsupported in Firebird backend
    throw SOCIError("RowIDs are not supported");
}

FirebirdRowIDBackEnd::~FirebirdRowIDBackEnd()
{
    // Unsupported in Firebird backend
    throw SOCIError("RowIDs are not supported");
}
