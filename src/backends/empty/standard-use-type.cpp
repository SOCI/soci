//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/empty/soci-empty.h"

using namespace soci;
using namespace soci::details;


void empty_standard_use_type_backend::bind_by_pos(
    int & /* position */, void * /* data */,
    exchange_type /* type */, bool /* readOnly */)
{
    // ...
}

void empty_standard_use_type_backend::bind_by_name(
    std::string const & /* name */, void * /* data */,
    exchange_type /* type */, bool /* readOnly */)
{
    // ...
}

void empty_standard_use_type_backend::pre_use(indicator const * /* ind */)
{
    // ...
}

void empty_standard_use_type_backend::post_use(
    bool /* gotData */, indicator * /* ind */)
{
    // ...
}

void empty_standard_use_type_backend::clean_up()
{
    // ...
}
