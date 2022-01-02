//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/blob.h"
#include "soci/session.h"

#include <cstddef>

using namespace soci;

blob::blob(): backEnd_(NULL)
{ }

blob::blob(session & s): backEnd_(s.make_blob_backend())
{ }

blob::~blob()
{
    delete backEnd_;
}

std::size_t blob::get_len()
{
    return data_.size();
}

std::size_t blob::read(std::size_t offset, char * buf, std::size_t toRead)
{
    std::size_t size = data_.size();

    if (size < offset)
    {
        throw soci_error("Can't read past-the-end of BLOB data");
    }

    std::size_t limit = std::min(size - offset, toRead);

    memcpy(buf, &data_[offset], limit);
    
    return limit;
}

std::size_t blob::write(std::size_t offset, char const * buf, std::size_t toWrite)
{
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

    memcpy(&data_[offset], buf, toWrite);

    return toWrite;
}

std::size_t blob::append(char const * buf, std::size_t toWrite)
{
    std::size_t size = data_.size();
    data_.resize(size + toWrite);

    memcpy(&data_[size], buf, toWrite);

    return toWrite;
}

void blob::trim(std::size_t newLen)
{
    if(newLen < data_.size())
    {
        data_.resize(newLen);
    }
}
