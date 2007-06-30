//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci-sqlite3.h"

using namespace soci;

sqlite3_blob_backend::sqlite3_blob_backend(sqlite3_session_backend &session)
    : session_(session)
{
    throw soci_error("BLOBs are not supported.");
}

sqlite3_blob_backend::~sqlite3_blob_backend()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t sqlite3_blob_backend::get_len()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t sqlite3_blob_backend::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t sqlite3_blob_backend::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t sqlite3_blob_backend::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

void sqlite3_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("BLOBs are not supported.");
}

