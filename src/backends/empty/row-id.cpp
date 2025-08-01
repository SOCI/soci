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


empty_rowid_backend::empty_rowid_backend(empty_session_backend & /* session */)
{
    // ...
}

empty_rowid_backend::~empty_rowid_backend()
{
    // ...
}
