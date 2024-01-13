//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"

#include <algorithm>
#include <cstring>

using namespace soci;

sqlite3_blob_backend::sqlite3_blob_backend(sqlite3_session_backend &)
    : details::trivial_blob_backend()
{
}

sqlite3_blob_backend::~sqlite3_blob_backend()
{
}

void sqlite3_blob_backend::ensure_buffer_initialized()
{
    // Ensure that the used buffer is at least large enough to hold one element.
    // Thus, in case the vector has not yet allocated a buffer at all, it is forced
    // to do so now.
    buffer_.reserve(1);
}
