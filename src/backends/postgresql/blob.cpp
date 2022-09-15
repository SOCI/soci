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


postgresql_blob_backend::blob_details::blob_details() : oid(InvalidOid), fd(-1) {}

postgresql_blob_backend::blob_details::blob_details(unsigned long oid, int fd) : oid(oid), fd(fd) {}


postgresql_blob_backend::postgresql_blob_backend(
    postgresql_session_backend & session)
    : session_(session), details_(), destroy_on_close_(false)
{
    // nothing to do here, the descriptor is open in the postFetch
    // method of the Into element
}

postgresql_blob_backend::~postgresql_blob_backend()
{
    reset();
}

std::size_t postgresql_blob_backend::get_len()
{
    return seek(0, SEEK_END);
}

std::size_t postgresql_blob_backend::read_from_start(char * buf, std::size_t toRead, std::size_t offset)
{
    seek(offset, SEEK_SET);

    int const readn = lo_read(session_.conn_, details_.fd, buf, toRead);
    if (readn < 0)
    {
        throw soci_error("Cannot read from BLOB.");
    }

    return static_cast<std::size_t>(readn);
}

std::size_t postgresql_blob_backend::write_from_start(char const * buf, std::size_t toWrite, std::size_t offset)
{
    init();

    seek(offset, SEEK_SET);

    int const written = lo_write(session_.conn_, details_.fd,
        const_cast<char *>(buf), toWrite);
    if (written < 0)
    {
        throw soci_error("Cannot write to BLOB.");
    }

    return static_cast<std::size_t>(written);
}

std::size_t postgresql_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    init();

    seek(0, SEEK_END);

    int const writen = lo_write(session_.conn_, details_.fd,
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
    int ret_code = lo_truncate64(session_.conn_, details_.fd, newLen);
# else
    int ret_code = -1;
# endif
    if (ret_code == -1) {
        // If we call lo_truncate64 on a server that is < v9.3, the call will fail and return -1.
        // Thus, we'll try again with the slightly older function lo_truncate.
        ret_code = lo_truncate(session_.conn_, details_.fd, newLen);
    }

    if (ret_code < 0) {
        throw soci_error("Cannot truncate BLOB");
    }
#endif
}

const postgresql_blob_backend::blob_details &postgresql_blob_backend::get_blob_details() const {
    return details_;
}

void postgresql_blob_backend::set_blob_details(const postgresql_blob_backend::blob_details &details) {
    reset();

    details_ = details;
}

bool postgresql_blob_backend::get_destroy_on_close() const {
    return destroy_on_close_;
}

void postgresql_blob_backend::set_destroy_on_close(bool destroy) {
    destroy_on_close_ = destroy;
}

std::size_t postgresql_blob_backend::seek(std::size_t toOffset, int from) {
#if PG_VERSION_NUM >= 90003
    pg_int64 pos = lo_lseek64(session_.conn_, details_.fd, static_cast<pg_int64>(toOffset), from);
#else
    int pos = -1;
#endif
    if (pos == -1) {
        // If we try to use lo_lseek64 on a Postgresql server that is older than 9.3, the function will fail
        // and return -1, so we'll try again with the older function lo_lseek.
        pos = lo_lseek(session_.conn_, details_.fd, static_cast<int>(toOffset), from);
    }

    if (pos < 0)
    {
        throw soci_error("Cannot retrieve the size of BLOB.");
    }

    return static_cast<std::size_t>(pos);
}

void postgresql_blob_backend::init() {
    if (details_.fd == -1) {
        // Create a new large object
        Oid oid = lo_creat(session_.conn_, INV_READ | INV_WRITE);

        if (oid == InvalidOid) {
            throw soci_error("Cannot create new BLOB.");
        }

        int fd = lo_open(session_.conn_, oid, INV_READ | INV_WRITE);

        if (fd == -1) {
            lo_unlink(session_.conn_, oid);
            throw soci_error("Cannot open newly created BLOB.");
        }

        details_.oid = oid;
        details_.fd = fd;
    }
}

void postgresql_blob_backend::reset() {
    if (details_.fd != -1)
    {
        if (destroy_on_close_) {
            // Remove the large object from the DB completely
            lo_unlink(session_.conn_, details_.fd);
        } else {
            // Merely close our handle to the large object
            lo_close(session_.conn_, details_.fd);
        }
    }

    destroy_on_close_ = false;
}
