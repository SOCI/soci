//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-sqlite3.h"

using namespace SOCI;

Sqlite3BLOBBackEnd::Sqlite3BLOBBackEnd(Sqlite3SessionBackEnd &session)
    : session_(session)
{
    throw SOCIError("BLOBs are not supported.");
}

Sqlite3BLOBBackEnd::~Sqlite3BLOBBackEnd()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t Sqlite3BLOBBackEnd::getLen()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t Sqlite3BLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t Sqlite3BLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t Sqlite3BLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

void Sqlite3BLOBBackEnd::trim(std::size_t /* newLen */)
{
    throw SOCIError("BLOBs are not supported.");
}

