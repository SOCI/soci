//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/odbc/soci-odbc.h"

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4702)
#endif

using namespace soci;
using namespace soci::details;


odbc_blob_backend::odbc_blob_backend(odbc_session_backend &session)
    : session_(session)
{
    throw soci_error("BLOBs are not supported.");
}

odbc_blob_backend::~odbc_blob_backend()
{
}

std::size_t odbc_blob_backend::get_len()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t odbc_blob_backend::read_from_start(void * /* buf */, std::size_t /* toRead */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t odbc_blob_backend::write_from_start(const void * /* buf */, std::size_t /* toWrite */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t odbc_blob_backend::append(
    const void * /* buf */, std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

void odbc_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("BLOBs are not supported.");
}

details::session_backend &odbc_blob_backend::get_session_backend()
{
    return session_;
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
