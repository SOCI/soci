//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/blob.h"
#include "soci/session.h"
#include "soci/soci-platform.h"

#include <cstddef>
#include <cassert>

using namespace soci;

blob::blob(session & s)
	: backEnd_{s.make_blob_backend()}
{
}

blob::~blob() = default;

std::size_t blob::get_len()
{
    assert(backEnd_);
    return backEnd_->get_len();
}

std::size_t blob::read(std::size_t offset, char *buf, std::size_t toRead)
{
    assert(backEnd_);
    SOCI_ALLOW_DEPRECATED_BEGIN
    return backEnd_->read(offset, buf, toRead);
    SOCI_ALLOW_DEPRECATED_END
}

std::size_t blob::read_from_start(char * buf, std::size_t toRead,
    std::size_t offset)
{
    assert(backEnd_);
    return backEnd_->read_from_start(buf, toRead, offset);
}

std::size_t blob::write(
    std::size_t offset, char const * buf, std::size_t toWrite)
{
    assert(backEnd_);
    SOCI_ALLOW_DEPRECATED_BEGIN
    return backEnd_->write(offset, buf, toWrite);
    SOCI_ALLOW_DEPRECATED_END
}

std::size_t blob::write_from_start(const char * buf, std::size_t toWrite,
    std::size_t offset)
{
    assert(backEnd_);
    return backEnd_->write_from_start(buf, toWrite, offset);
}

std::size_t blob::append(char const * buf, std::size_t toWrite)
{
    assert(backEnd_);
    return backEnd_->append(buf, toWrite);
}

void blob::trim(std::size_t newLen)
{
    assert(backEnd_);
    backEnd_->trim(newLen);
}
