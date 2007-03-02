//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "ref-counted-statement.h"
#include "statement.h"

using namespace soci;
using namespace soci::details;

void ref_counted_statement::final_action()
{
    try
    {
        st_.alloc();
        st_.prepare(query_.str(), eOneTimeQuery);
        st_.define_and_bind();
        st_.execute(true);
    }
    catch (...)
    {
        st_.clean_up();
        throw;
    }

    st_.clean_up();
}
