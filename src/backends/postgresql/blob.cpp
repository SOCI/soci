//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"
#include <libpq/libpq-fs.h> // libpq
#include <pg_config.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <limits>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


postgresql_blob_backend::postgresql_blob_backend(
    postgresql_session_backend & session)
    : session_(session), fd_(-1)
{
    // nothing to do here, the descriptor is open in the postFetch
    // method of the Into element
}

postgresql_blob_backend::~postgresql_blob_backend()
{
    if (fd_ != -1)
    {
        lo_close(session_.conn_, fd_);
    }
}

std::size_t postgresql_blob_backend::get_len()
{
    int const pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw soci_error("Cannot retrieve the size of BLOB.");
    }

    return static_cast<std::size_t>(pos);
}

std::size_t postgresql_blob_backend::read_from_start(char * buf, std::size_t toRead, std::size_t offset)
{
    int const pos = lo_lseek(session_.conn_, fd_,
        static_cast<int>(offset), SEEK_SET);
    if (pos == -1)
    {
        throw soci_error("Cannot seek in BLOB.");
    }

    int const readn = lo_read(session_.conn_, fd_, buf, toRead);
    if (readn < 0)
    {
        throw soci_error("Cannot read from BLOB.");
    }

    return static_cast<std::size_t>(readn);
}

std::size_t postgresql_blob_backend::write_from_start(char const * buf, std::size_t toWrite, std::size_t offset)
{
    int const pos = lo_lseek(session_.conn_, fd_,
        static_cast<int>(offset), SEEK_SET);
    if (pos == -1)
    {
        throw soci_error("Cannot seek in BLOB.");
    }

    int const writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw soci_error("Cannot write to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

std::size_t postgresql_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    int const pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw soci_error("Cannot seek in BLOB.");
    }

    int const writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw soci_error("Cannot append to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

void postgresql_blob_backend::trim(std::size_t newLen)
{
#if PG_VERSION_NUM < 80003
    // lo_truncate was introduced in Postgresql v8.3
    (void) newLen;
    throw soci_error("Your Postgresql version does not support trimming BLOBs");
#else
    if (newLen > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw soci_error("Request new BLOB size exceeds INT_MAX, which is not supported");
    }

# if PG_VERSION_NUM >= 90003
    // lo_truncate64 was introduced in Postgresql v9.3
    int ret_code = lo_truncate64(session_.conn_, fd_, newLen);
# else
    int ret_code = -1;
# endif
    if (ret_code == -1) {
        // If we call lo_truncate64 on a server that is < v9.3, the call will fail and return -1.
        // Thus, we'll try again with the slightly older function lo_truncate.
        ret_code = lo_truncate(session_.conn_, fd_, newLen);
    }

    if (ret_code < 0) {
        throw soci_error("Cannot truncate BLOB");
    }
#endif
}
