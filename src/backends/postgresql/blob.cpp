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

std::size_t postgresql_blob_backend::read_from_start(char * buf, std::size_t toRead, std::size_t offset)
{
    std::size_t size = get_len();
    if (offset >= size && !(size == 0 && offset == 0)) {
        throw soci_error("Can't read past-the-end of BLOB data.");
    }
    if (size == 0) {
        // Reading from an empty blob, is defined as a no-op
        return 0;
    }

    seek(offset, SEEK_SET);

    int const readn = lo_read(session_.conn_, details_.fd, buf, toRead);
    if (readn < 0)
    {
        throw soci_error(std::string("Cannot read from BLOB: ") + PQerrorMessage(session_.conn_));
    }

    return static_cast<std::size_t>(readn);
}

std::size_t postgresql_blob_backend::write_from_start(char const * buf, std::size_t toWrite, std::size_t offset)
{
    if (offset > get_len())
    {
        // If offset == length, the operation is to be understood as appending (and is therefore allowed)
        throw soci_error("Can't start writing far past-the-end of BLOB data.");
    }

    if (clone_before_modify_) {
        clone();
    }

    init();

    seek(offset, SEEK_SET);

    int const written = lo_write(session_.conn_, details_.fd,
        const_cast<char *>(buf), toWrite);
    if (written < 0)
    {
        throw soci_error(std::string("Cannot write to BLOB: ") + PQerrorMessage(session_.conn_));
    }

    return static_cast<std::size_t>(written);
}

std::size_t postgresql_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    return write_from_start(buf, toWrite, get_len());
}

void postgresql_blob_backend::trim(std::size_t newLen)
{
    if (newLen > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw soci_error("Request new BLOB size exceeds INT_MAX, which is not supported");
    }

    if (clone_before_modify_) {
        if (newLen == 0) {
            // In case we're clearing the BLOB anyway, there is no point in copying the old data over first
            reset();
        } else {
            clone();
        }
    }

    init();

    // lo_truncate64 was introduced in Postgresql v9.3
    int ret_code = lo_truncate64(session_.conn_, details_.fd, newLen);
    if (ret_code == -1) {
        // If we call lo_truncate64 on a server that is < v9.3, the call will fail and return -1.
        // Thus, we'll try again with the slightly older function lo_truncate.
        ret_code = lo_truncate(session_.conn_, details_.fd, newLen);
    }

    if (ret_code < 0) {
        throw soci_error(std::string("Cannot truncate BLOB: ") + PQerrorMessage(session_.conn_));
    }
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

void postgresql_blob_backend::set_clone_before_modify(bool clone) {
    clone_before_modify_ = clone;
}

void postgresql_blob_backend::init() {
    if (details_.fd == -1) {
        // Create a new large object
        Oid oid = lo_creat(session_.conn_, INV_READ | INV_WRITE);

        if (oid == InvalidOid) {
            throw soci_error(std::string("Cannot create new BLOB: ") + PQerrorMessage(session_.conn_));
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

        details_ = {};
    }

    destroy_on_close_ = false;
    clone_before_modify_ = false;
}

std::size_t do_seek(std::size_t toOffset, int from,
        soci::postgresql_session_backend &session, soci::postgresql_blob_backend::blob_details &details)
{
    pg_int64 pos = lo_lseek64(session.conn_, details.fd, static_cast<pg_int64>(toOffset), from);
    if (pos == -1) {
        // If we try to use lo_lseek64 on a Postgresql server that is older than 9.3, the function will fail
        // and return -1, so we'll try again with the older function lo_lseek.
        pos = lo_lseek(session.conn_, details.fd, static_cast<int>(toOffset), from);
    }

    if (pos < 0)
    {
        throw soci_error("Cannot retrieve the seek in BLOB.");
    }

    return static_cast<std::size_t>(pos);
}

std::size_t postgresql_blob_backend::seek(std::size_t toOffset, int from) {
    return do_seek(toOffset, from, session_, details_);
}

void postgresql_blob_backend::clone() {
    clone_before_modify_ = false;
    if (details_.fd == -1) {
        return;
    }

    blob_details old_details = details_;
	details_ = {};
    reset();
    init();

    char buf[1024];
    std::size_t offset = 0;
    int read_bytes = 0;
    do {
        do_seek(offset, SEEK_SET, session_, old_details);
        read_bytes = lo_read(session_.conn_, old_details.fd, buf, sizeof(buf));

        if (read_bytes < 0)
        {
            throw soci::soci_error("Can't read from original BLOB during clone");
        }

        int bytes_written = lo_write(session_.conn_, details_.fd, buf, read_bytes);

        if (bytes_written != read_bytes)
        {
            throw soci::soci_error("Can't write (all) data from old BLOB to new one during clone");
        }

        offset += sizeof(buf);
    } while (read_bytes == sizeof(buf));

	// Dispose old BLOB object
	if (destroy_on_close_) {
		// Remove the large object from the DB completely
		lo_unlink(session_.conn_, old_details.fd);
	} else {
		// Merely close our handle to the large object
		lo_close(session_.conn_, old_details.fd);
	}
}
