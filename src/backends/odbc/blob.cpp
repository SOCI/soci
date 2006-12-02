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


ODBCBLOBBackEnd::ODBCBLOBBackEnd(ODBCSessionBackEnd &session)
    : session_(session)
{
    // ...
}

ODBCBLOBBackEnd::~ODBCBLOBBackEnd()
{
    // ...
}

std::size_t ODBCBLOBBackEnd::getLen()
{
    // ...
    return 0;
}

std::size_t ODBCBLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t ODBCBLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t ODBCBLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void ODBCBLOBBackEnd::trim(std::size_t /* newLen */)
{
    // ...
}
