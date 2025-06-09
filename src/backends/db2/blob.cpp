//
// Copyright (C) 2011-2013 Denis Chapligin
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/db2/soci-db2.h"

#ifdef _MSC_VER
# pragma warning(disable:4355 4702)
#endif

using namespace soci;
using namespace soci::details;


db2_blob_backend::db2_blob_backend(db2_session_backend &session)
    : session_(session)
{
    throw soci_error("BLOBs are not supported.");
}

db2_blob_backend::~db2_blob_backend()
{
}

std::size_t db2_blob_backend::get_len()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t db2_blob_backend::read_from_start(void * /* buf */, std::size_t /* toRead */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t db2_blob_backend::write_from_start(const void * /* buf */, std::size_t /* toWrite */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t db2_blob_backend::append(
    const void * /* buf */, std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

void db2_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("BLOBs are not supported.");
}

details::session_backend &db2_blob_backend::get_session_backend()
{
    return session_;
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
