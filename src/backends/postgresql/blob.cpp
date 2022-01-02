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
    : session_(session), oid_(0u), fd_(-1)
{ }

postgresql_blob_backend::~postgresql_blob_backend()
{
    this->close();
}

void postgresql_blob_backend::assign(details::holder* h)
{
    this->assign(h->get<int>());
}

void postgresql_blob_backend::assign(unsigned int const & oid)
{
    close();
    oid_ = oid;
}

void postgresql_blob_backend::read(blob &b)
{
    this->open(INV_READ);
    b.resize(this->get_len());
    this->read_from_start(&b[0], b.size());
    this->close();
}

void postgresql_blob_backend::write(blob &b) 
{
    this->open(INV_WRITE);
    this->write_from_start(&b[0], b.size());
    this->close();
}

int postgresql_blob_backend::open(int mode)
{
    if (oid_ == 0u)
    {
        oid_ = lo_create(session_.conn_, 0);
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

    return ret;
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

std::size_t postgresql_blob_backend::read(
    std::size_t offset, char * buf, std::size_t toRead)
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

std::size_t postgresql_blob_backend::write(
    std::size_t offset, char const * buf, std::size_t toWrite)
{
    if (toWrite == 0)
    {
        return static_cast<std::size_t>(toWrite);
    }

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
    if (toWrite == 0)
    {
        return static_cast<std::size_t>(toWrite);
    }

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

void postgresql_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("Trimming BLOBs is not supported.");
}