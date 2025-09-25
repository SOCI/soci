//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/blob.h"
#include "soci/error.h"
#include "soci/session.h"

#include <cstddef>

using namespace soci;

blob::blob(session & s)
{
    initialize(s);
}

blob::~blob() = default;

bool blob::is_valid() const
{
    return static_cast<bool>(backEnd_);
}

void blob::initialize(session &session)
{
    initialize(session.make_blob_backend());
}

void blob::initialize(details::blob_backend *backend)
{
    backEnd_.reset(backend);
}

std::size_t blob::get_len() const
{
    ensure_initialized();
    return backEnd_->get_len();
}

std::size_t blob::read(std::size_t offset, void *buf, std::size_t toRead) const
{
    ensure_initialized();
    return backEnd_->read(offset, buf, toRead);
}

std::size_t blob::read_from_start(void * buf, std::size_t toRead,
    std::size_t offset) const
{
    ensure_initialized();
    return backEnd_->read_from_start(buf, toRead, offset);
}

std::size_t blob::write(
    std::size_t offset, const void * buf, std::size_t toWrite) const
{
    ensure_initialized();
    return backEnd_->write(offset, buf, toWrite);
}

std::size_t blob::write_from_start(const void * buf, std::size_t toWrite,
    std::size_t offset) const
{
    ensure_initialized();
    return backEnd_->write_from_start(buf, toWrite, offset);
}

std::size_t blob::append(const void * buf, std::size_t toWrite) const
{
    ensure_initialized();
    return backEnd_->append(buf, toWrite);
}

void blob::trim(std::size_t newLen) const
{
    ensure_initialized();
    backEnd_->trim(newLen);
}

void blob::ensure_initialized() const
{
    if (!is_valid())
    {
        throw soci_error("Attempted to access invalid blob");
    }
}
