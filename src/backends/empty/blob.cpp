//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_EMPTY_SOURCE
#include "soci/empty/soci-empty.h"

using namespace soci;
using namespace soci::details;


empty_blob_backend::empty_blob_backend(empty_session_backend &session)
    : session_(session)
{
    throw soci_error("BLOBs are not supported.");
}

empty_blob_backend::~empty_blob_backend()
{
}

std::size_t empty_blob_backend::get_len()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t empty_blob_backend::read_from_start(void * /* buf */, std::size_t /* toRead */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t empty_blob_backend::write_from_start(const void * /* buf */, std::size_t /* toWrite */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t empty_blob_backend::append(
    const void * /* buf */, std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

void empty_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("BLOBs are not supported.");
}

details::session_backend &empty_blob_backend::get_session_backend()
{
    return session_;
}
