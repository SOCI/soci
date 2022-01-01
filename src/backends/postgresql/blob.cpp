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

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


postgresql_blob_backend::postgresql_blob_backend(
    postgresql_session_backend & session)
    : session_(session), oid_(0u), fd_(-1), from_db_(false), loaded_(false)
{ }

postgresql_blob_backend::~postgresql_blob_backend()
{
    this->close();
}

void postgresql_blob_backend::assign(unsigned int const & oid)
{
    close();

    oid_ = oid;
    from_db_ = true;
}

int postgresql_blob_backend::open(int mode)
{
    if (oid_ == 0u)
    {
        return 0;
    }

    fd_ = lo_open(session_.conn_, oid_, mode);

    if (fd_ == -1)
    {
        throw soci_error("Cannot open BLOB.");
    }

    return fd_;
}

int postgresql_blob_backend::close()
{
    if (fd_ == -1)
    {
        return 0;
    }

    int ret = lo_close(session_.conn_, fd_);

    if (ret < 0)
    {
        throw soci_error("Cannot close BLOB.");
    }

    fd_ = -1;
    loaded_ = false;

    return ret;
}

void postgresql_blob_backend::load()
{
    if (fd_ == -1)
    {
        this->open(INV_READ);
    }

    int blob_size = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    lo_lseek(session_.conn_, fd_, 0, SEEK_SET);
    
    if (blob_size == -1)
    {
        this->close();
        throw soci_error("Cannot retrieve the size of BLOB.");
    }

    data_.resize(blob_size);

    // The blob is empty.
    if (data_.size() == 0)
    {
        this->close();
        loaded_ = true;
        return;
    }

    int const readn = lo_read(session_.conn_, fd_, &data_[0], data_.size());
    if (readn < 0)
    {
        this->close();
        throw soci_error("Cannot read from BLOB.");
    }

    this->close();
    loaded_ = true;
}

void postgresql_blob_backend::save()
{
    close();
    // if (oid_ != 0u)
    // {
    //     if (lo_unlink(session_.conn_, oid_) < 0)
    //     {
    //         throw soci_error("Cannot unlink old BLOB.");
    //     }
    //     oid_ = 0;
    // }

    oid_ = lo_create(session_.conn_, 0);
    if (oid_ == 0)
    {
        throw soci_error("Cannot create a new BLOB.");
    }

    if (data_.size() == 0)
    {
        return;
    }

    this->open(INV_WRITE);
    int const writen = lo_write(session_.conn_, fd_, &data_[0], data_.size());
    this->close();

    if (writen < 0)
    {
        throw soci_error("Cannot write to BLOB.");
    }
}

std::size_t postgresql_blob_backend::get_len()
{
    if (from_db_ && (loaded_ == false))
    {
        load();
    }
    return data_.size();
}

std::size_t postgresql_blob_backend::read(
    std::size_t offset, char * buf, std::size_t toRead)
{
    if (from_db_ && (loaded_ == false))
    {
        load();
    }

    std::size_t size = data_.size();

    if (offset > size)
    {
        throw soci_error("Can't read past-the-end of BLOB data");
    }

    char * itr = buf;
    std::size_t limit = std::min(size - offset, toRead);
    std::size_t index = 0;

    while (index < limit)
    {
        *itr = data_[offset+index];
        ++index;
        ++itr;
    }

    return limit;
}

std::size_t postgresql_blob_backend::write(
    std::size_t offset, char const * buf, std::size_t toWrite)
{
    if (from_db_ && (loaded_ == false))
    {
        load();
    }

    std::size_t size = data_.size();

    if (offset > size)
    {
        throw soci_error("Can't write past-the-end of BLOB data");
    }

    // make sure there is enough space in buffer
    if (toWrite > (size - offset))
    {
        data_.resize(size + (toWrite - (size - offset)));
    }

    writeBuffer(offset, buf, toWrite);

    return toWrite;
}

std::size_t postgresql_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    if (from_db_ && (loaded_ == false))
    {
        // this is blob fetched from database, but not loaded yet
        load();
    }

    std::size_t size = data_.size();
    data_.resize(size + toWrite);

    writeBuffer(size, buf, toWrite);

    return toWrite;
}

void postgresql_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("Trimming BLOBs is not supported.");
}

void postgresql_blob_backend::writeBuffer(std::size_t offset,
                                      char const * buf, std::size_t toWrite)
{
    char const * itr = buf;
    char const * end_itr = buf + toWrite;

    while (itr!=end_itr)
    {
        data_[offset++] = *itr++;
    }
}
