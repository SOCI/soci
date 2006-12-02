//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include <soci.h>
#include <libpq/libpq-fs.h> // libpq
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


PostgreSQLBLOBBackEnd::PostgreSQLBLOBBackEnd(
    PostgreSQLSessionBackEnd &session)
    : session_(session), fd_(-1)
{
    // nothing to do here, the descriptor is open in the postFetch
    // method of the Into element
}

PostgreSQLBLOBBackEnd::~PostgreSQLBLOBBackEnd()
{
    lo_close(session_.conn_, fd_);
}

std::size_t PostgreSQLBLOBBackEnd::getLen()
{
    int pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw SOCIError("Cannot retrieve the size of BLOB.");
    }

    return static_cast<std::size_t>(pos);
}

std::size_t PostgreSQLBLOBBackEnd::read(
    std::size_t offset, char *buf, std::size_t toRead)
{
    int pos = lo_lseek(session_.conn_, fd_,
        static_cast<int>(offset), SEEK_SET);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int readn = lo_read(session_.conn_, fd_, buf, toRead);
    if (readn < 0)
    {
        throw SOCIError("Cannot read from BLOB.");
    }

    return static_cast<std::size_t>(readn);
}

std::size_t PostgreSQLBLOBBackEnd::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    int pos = lo_lseek(session_.conn_, fd_,
        static_cast<int>(offset), SEEK_SET);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw SOCIError("Cannot write to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

std::size_t PostgreSQLBLOBBackEnd::append(
    char const *buf, std::size_t toWrite)
{
    int pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw SOCIError("Cannot append to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

void PostgreSQLBLOBBackEnd::trim(std::size_t /* newLen */)
{
    throw SOCIError("Trimming BLOBs is not supported.");
}
