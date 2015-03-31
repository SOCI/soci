//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2015 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE

#include "soci/error.h"

using namespace soci;

soci_error::soci_error(std::string const & msg)
     : std::runtime_error(msg)
{
}

std::string soci_error::get_error_message() const
{
    return std::runtime_error::what();
}
