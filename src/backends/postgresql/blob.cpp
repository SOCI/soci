//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdint>
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

// We need this helper function when using ancient PostgreSQL versions without
// support for 64-bit offets.
#ifdef SOCI_POSTGRESQL_NO_LO64
static int pg_check_fits_32_bits(std::size_t len)
{
    if (len > 0x7fffffff)
    {
        throw soci_error("64-bit offsets support requires PostgreSQL 9.3 or later");
    }

    return static_cast<int>(len);
}
#endif // PostgreSQL < 9.3


postgresql_blob_backend::blob_details::blob_details() : oid(InvalidOid), fd(-1) {}

postgresql_blob_backend::blob_details::blob_details(unsigned long oid, int fd) : oid(oid), fd(fd) {}


postgresql_blob_backend::postgresql_blob_backend(
    postgresql_session_backend & session)
    : session_(session), details_(), destroy_on_close_(false), clone_before_modify_(false)
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
    return details_.fd == -1 ? 0 : seek(0, SEEK_END);
}

std::size_t postgresql_blob_backend::read_from_start(void * buf, std::size_t toRead, std::size_t offset)
{
    std::size_t size = get_len();
    if (offset >= size && !(size == 0 && offset == 0))
    {
        throw soci_error("Can't read past-the-end of BLOB data.");
    }
    if (size == 0)
    {
        // Reading from an empty blob, is defined as a no-op
        return 0;
    }

    // According to postgres docs, lo_read rejects anything that wants to read more than INT_MAX bytes
    const std::size_t batchSize = std::numeric_limits<std::int32_t>::max();
    const std::size_t nBatches = toRead / batchSize + 1;

    std::size_t readBytes = 0;
    for (std::size_t i = 0; i < nBatches; ++i) {
        seek(offset, SEEK_SET);

        const int readn = lo_read(session_.conn_, details_.fd, reinterpret_cast<char *>(buf), std::min(toRead, batchSize));
        if (readn < 0)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            throw soci_error(std::string("Cannot read from BLOB: ") + errorMsg);
        }

        toRead -= batchSize;
        offset += batchSize;
        readBytes += static_cast<std::size_t>(readn);
    }

    return readBytes;
}

std::size_t postgresql_blob_backend::write_from_start(const void * buf, std::size_t toWrite, std::size_t offset)
{
    if (offset > get_len())
    {
        // If offset == length, the operation is to be understood as appending (and is therefore allowed)
        throw soci_error("Can't start writing far past-the-end of BLOB data.");
    }

    if (clone_before_modify_)
    {
        clone();
    }

    init();

    // According to the docs, lo_write doesn't work for chunk sizes > INT_MAX
    const std::size_t batchSize = std::numeric_limits<std::int32_t>::max();
    const std::size_t nBatches = toWrite / batchSize + 1;

    std::size_t writtenBytes = 0;
    for (std::size_t i = 0; i < nBatches; ++i) {
        seek(offset, SEEK_SET);

        const int written = lo_write(session_.conn_, details_.fd,
            reinterpret_cast<const char *>(buf), std::min(toWrite, batchSize));
        if (written < 0)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            throw soci_error(std::string("Cannot write to BLOB: ") + errorMsg);
        }

        toWrite -= batchSize;
        offset += batchSize;
        writtenBytes += static_cast<std::size_t>(written);
    }

    return writtenBytes;
}

std::size_t postgresql_blob_backend::append(
    const void * buf, std::size_t toWrite)
{
    return write_from_start(buf, toWrite, get_len());
}

void postgresql_blob_backend::trim(std::size_t newLen)
{
    if (clone_before_modify_)
    {
        if (newLen == 0)
        {
            // In case we're clearing the BLOB anyway, there is no point in copying the old data over first
            reset();
        }
        else
        {
            clone();
        }
    }

    init();

#ifndef SOCI_POSTGRESQL_NO_LO64
    int ret_code = lo_truncate64(session_.conn_, details_.fd, newLen);
#else
    int ret_code = lo_truncate(session_.conn_, details_.fd, pg_check_fits_32_bits(newLen));
#endif

    if (ret_code < 0)
    {
        const char *errorMsg = PQerrorMessage(session_.conn_);
        throw soci_error(std::string("Cannot truncate BLOB: ") + errorMsg);
    }
}

const postgresql_blob_backend::blob_details &postgresql_blob_backend::get_blob_details() const
{
    return details_;
}

void postgresql_blob_backend::set_blob_details(const postgresql_blob_backend::blob_details &details)
{
    reset();

    details_ = details;
}

bool postgresql_blob_backend::get_destroy_on_close() const
{
    return destroy_on_close_;
}

void postgresql_blob_backend::set_destroy_on_close(bool destroy)
{
    destroy_on_close_ = destroy;
}

void postgresql_blob_backend::set_clone_before_modify(bool clone)
{
    clone_before_modify_ = clone;
}

void postgresql_blob_backend::init()
{
    if (details_.fd == -1)
    {
        // Create a new large object
        Oid oid = lo_creat(session_.conn_, INV_READ | INV_WRITE);

        if (oid == InvalidOid)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            throw soci_error(std::string("Cannot create new BLOB: ") + errorMsg);
        }

        int fd = lo_open(session_.conn_, oid, INV_READ | INV_WRITE);

        if (fd == -1)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            lo_unlink(session_.conn_, oid);
            throw soci_error(std::string("Cannot open newly created BLOB: ") + errorMsg);
        }

        details_.oid = oid;
        details_.fd = fd;
    }
}

void postgresql_blob_backend::reset()
{
    if (details_.fd != -1)
    {
        if (destroy_on_close_)
        {
            // Remove the large object from the DB completely
            lo_unlink(session_.conn_, details_.fd);
        }
        else
        {
            // Merely close our handle to the large object
            lo_close(session_.conn_, details_.fd);
        }

        details_ = {};
    }

    destroy_on_close_ = false;
    clone_before_modify_ = false;
}

std::size_t do_seek(std::size_t toOffset, int from,
        soci::postgresql_session_backend &session, soci::postgresql_blob_backend::blob_details &details)
{
#ifndef SOCI_POSTGRESQL_NO_LO64
    pg_int64 pos = lo_lseek64(session.conn_, details.fd, static_cast<pg_int64>(toOffset), from);
#else
    int pos = lo_lseek(session.conn_, details.fd, pg_check_fits_32_bits(toOffset), from);
#endif

    if (pos < 0)
    {
        const char *errorMsg = PQerrorMessage(session.conn_);
        throw soci_error(std::string("Failed to seek in BLOB: ") + errorMsg);
    }

    return static_cast<std::size_t>(pos);
}

std::size_t postgresql_blob_backend::seek(std::size_t toOffset, int from)
{
    return do_seek(toOffset, from, session_, details_);
}

void postgresql_blob_backend::clone()
{
    clone_before_modify_ = false;
    if (details_.fd == -1)
    {
        return;
    }

    blob_details old_details = details_;
    details_ = {};
    reset();
    init();

    char buf[1024];
    std::size_t offset = 0;
    int read_bytes = 0;
    do
    {
        do_seek(offset, SEEK_SET, session_, old_details);
        read_bytes = lo_read(session_.conn_, old_details.fd, buf, sizeof(buf));

        if (read_bytes < 0)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            throw soci::soci_error(std::string("Can't read from original BLOB during clone: ") + errorMsg);
        }

        int bytes_written = lo_write(session_.conn_, details_.fd, buf, read_bytes);

        if (bytes_written != read_bytes)
        {
            const char *errorMsg = PQerrorMessage(session_.conn_);
            throw soci::soci_error(std::string("Can't write (all) data from old BLOB to new one during clone: ") + errorMsg);
        }

        offset += sizeof(buf);
    } while (read_bytes == sizeof(buf));

    // Dispose old BLOB object
    if (destroy_on_close_)
    {
        // Remove the large object from the DB completely
        lo_unlink(session_.conn_, old_details.fd);
    }
    else
    {
        // Merely close our handle to the large object
        lo_close(session_.conn_, old_details.fd);
    }
}
