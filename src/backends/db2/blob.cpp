//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2011 Denis Chapligin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_DB2_SOURCE
#include "soci-db2.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


db2_blob_backend::db2_blob_backend(db2_session_backend &session)
    : session_(session)
{
    // ...
}

db2_blob_backend::~db2_blob_backend()
{
    // ...
}

std::size_t db2_blob_backend::get_len()
{
    // ...
    return 0;
}

std::size_t db2_blob_backend::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t db2_blob_backend::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t db2_blob_backend::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void db2_blob_backend::trim(std::size_t /* newLen */)
{
    // ...
}
