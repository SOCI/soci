//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"

#include <algorithm>
#include <cstring>

using namespace soci;

sqlite3_blob_backend::sqlite3_blob_backend(sqlite3_session_backend &session)
    : session_(session), data_(), statement_(NULL), pos_(-1)
{ }

sqlite3_blob_backend::~sqlite3_blob_backend()
{ }

void sqlite3_blob_backend::assign(sqlite3_statement_backend *statement, int pos)
{
    statement_ = statement;
    pos_ = pos;
}

void sqlite3_blob_backend::assign(std::string data)
{
    // A row is trying to retrieve a BLOB, while for SQLite3, the BLOB is the string itself.
    data_ = data;
}

void sqlite3_blob_backend::assign(details::holder* h)
{
    this->assign(h->get<std::string>());
}

void sqlite3_blob_backend::read(blob& b)
{
    if (statement_ != NULL)
    {
        b.resize(this->get_len());
        this->read_from_start(&b[0], b.size());
    }
    else if(!data_.empty())
    {
        b.resize(data_.size());
        memcpy(&b[0], &data_[0], data_.size());
    }
}

void sqlite3_blob_backend::write(blob& b)
{
    this->write_from_start(&b[0], b.size());
}

std::size_t sqlite3_blob_backend::get_len()
{
    return static_cast<std::size_t>(sqlite3_column_bytes(statement_->stmt_, pos_));
}

std::size_t sqlite3_blob_backend::read(
    std::size_t offset, char * buf, std::size_t toRead)
{
    const char *buf_
        = reinterpret_cast<const char*>(
            sqlite3_column_blob(statement_->stmt_, pos_)
        );
        
    std::size_t cur_size = this->get_len();

    std::size_t newLen = (std::min)(cur_size - offset, toRead);

    memcpy(buf + offset, buf_, newLen);

    return newLen;
}


std::size_t sqlite3_blob_backend::write(
    std::size_t /*offset*/, char const * buf,
    std::size_t toWrite)
{
    sqlite3_column &col = statement_->useData_[0][pos_];

    col.buffer_.constData_ = buf;
    col.buffer_.size_ = toWrite;

    return toWrite;
}


std::size_t sqlite3_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    const char *oldBuf
        = reinterpret_cast<const char*>(
            sqlite3_column_blob(statement_->stmt_, pos_)
        );

    std::size_t cur_size = this->get_len();
    std::size_t new_size = cur_size + toWrite;

    char* tmp = new char[new_size];
    memcpy(tmp, oldBuf, cur_size);
    memcpy(tmp+cur_size, buf, toWrite);
    this->write(0, tmp, new_size);
    delete[] tmp;
    
    return new_size;
}


void sqlite3_blob_backend::trim(std::size_t newLen)
{
    std::size_t cur_size = this->get_len();

    if (cur_size < newLen)
    {
        throw soci_error("The trimmed size is bigger and the current blob size.");
    }
    
    const char *oldBuf
        = reinterpret_cast<const char*>(
            sqlite3_column_blob(statement_->stmt_, pos_)
        );

    char* tmp = new char[newLen];

    memcpy(tmp, oldBuf, newLen);
    this->write(0, tmp, newLen);

    delete [] tmp;
}
