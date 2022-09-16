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
    : session_(session), buffer_()
{
}

sqlite3_blob_backend::~sqlite3_blob_backend()
{
}

std::size_t sqlite3_blob_backend::get_len()
{
    return buffer_.size();
}

std::size_t sqlite3_blob_backend::read_from_start(char * buf, std::size_t toRead, std::size_t offset)
{
    toRead = (std::min)(toRead, buffer_.size() - offset);

    // make sure that we don't try to read
    // past the end of the data
    memcpy(buf, buffer_.data() + offset, toRead);

    return toRead;
}


std::size_t sqlite3_blob_backend::write_from_start(char const * buf, std::size_t toWrite, std::size_t offset)
{
    buffer_.resize((std::max)(buffer_.size(), offset + toWrite));

    memcpy(buffer_.data() + offset, buf, toWrite);

    return buffer_.size();
}


std::size_t sqlite3_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    return write_from_start(buf, toWrite, buffer_.size());
}


void sqlite3_blob_backend::trim(std::size_t newLen)
{
    buffer_.resize(newLen);
}

std::size_t sqlite3_blob_backend::set_data(char const *buf, std::size_t toWrite)
{
    buffer_.clear();
    return write_from_start(buf, toWrite);
}

const char *sqlite3_blob_backend::get_buffer() const
{
    return buffer_.data();
}
