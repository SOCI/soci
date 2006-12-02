//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci-mysql.h"
#include <soci.h>
#include <ciso646>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


MySQLBLOBBackEnd::MySQLBLOBBackEnd(MySQLSessionBackEnd &session)
    : session_(session)
{
    throw SOCIError("BLOBs are not supported.");
}

MySQLBLOBBackEnd::~MySQLBLOBBackEnd()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::getLen()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

void MySQLBLOBBackEnd::trim(std::size_t /* newLen */)
{
    throw SOCIError("BLOBs are not supported.");
}

