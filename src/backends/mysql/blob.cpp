//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"
#include <ciso646>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355 4702)
#endif

using namespace soci;
using namespace soci::details;

mysql_blob_backend::mysql_blob_backend(mysql_session_backend &session)
    : session_(session)
{
    throw soci_error("BLOBs are not supported.");
}

mysql_blob_backend::~mysql_blob_backend()
{
}

std::size_t mysql_blob_backend::get_len()
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t mysql_blob_backend::read_from_start(char * /* buf */, std::size_t /* toRead */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t mysql_blob_backend::write_from_start(char const * /* buf */, std::size_t /* toWrite */, std::size_t /* offset */)
{
    throw soci_error("BLOBs are not supported.");
}

std::size_t mysql_blob_backend::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    throw soci_error("BLOBs are not supported.");
}

void mysql_blob_backend::trim(std::size_t /* newLen */)
{
    throw soci_error("BLOBs are not supported.");
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
