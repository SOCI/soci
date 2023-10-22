//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/type-holder.h"

using namespace soci;
using namespace soci::details;



holder::holder(data_type dt_) : dt(dt_)
{
    switch (dt)
    {
        case soci::dt_blob:
        case soci::dt_xml:
        case soci::dt_string: new (&val.s) std::string();
        default: break;
    }
}

holder::~holder() {
    switch (dt)
    {
        case soci::dt_blob:
        case soci::dt_xml:
        case soci::dt_string: val.s.~basic_string();
        default: break;
    }
}

