//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci-sqlite3.h"
#include <cstring>

using namespace soci;

sqlite3_blob_backend::sqlite3_blob_backend(sqlite3_session_backend &session)
    : session_(session)
{
}

sqlite3_blob_backend::~sqlite3_blob_backend()
{
}

std::size_t sqlite3_blob_backend::get_len()
{
    return buf_.size();
}

std::size_t sqlite3_blob_backend::read(
    std::size_t offset, char * buf, std::size_t toRead)
{
    // make sure that we don't try to read
    // past the end of the data
    size_t len = buf_.size();
    if( offset >= len ) 
        offset = len;
    if (toRead > len - offset)
        toRead = len - offset;
    std::copy(&(*buf_.begin()) + offset, &(*buf_.begin()) + offset + toRead, buf);
    return toRead;
}


std::size_t sqlite3_blob_backend::write(
    std::size_t offset, char const * buf,
    std::size_t toWrite)
{
    if( buf_.size() < offset+toWrite ) //ensure that we have valid size of buffer before inserting
        buf_.resize(offset+toWrite);
    //write to offset 
    std::copy( buf, buf+toWrite,buf_.begin()+offset);
    return buf_.size();
}


std::size_t sqlite3_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    buf_.insert(buf_.end(), buf, buf+toWrite);
    return buf_.size();
}


void sqlite3_blob_backend::trim(std::size_t newLen)
{
    buf_.resize(newLen);
}

std::size_t sqlite3_blob_backend::set_data(char const *buf, std::size_t toWrite)
{
    buf_.resize(toWrite); //resize blob container
    return write(0, buf, toWrite);
}
