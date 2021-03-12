//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/ref-counted-statement.h"
#include "soci/session.h"

using namespace soci;
using namespace soci::details;

namespace
{

// Unfortunately we can't reuse details::auto_statement here because it works
// with statement_backend and not statement that we use here, so just define a
// similar class.
class auto_statement_alloc
{
public:
    explicit auto_statement_alloc(statement& st)
        : st_(st)
    {
        st_.alloc();
    }

    ~auto_statement_alloc()
    {
        st_.clean_up();
    }

private:
    statement& st_;

    SOCI_NOT_COPYABLE(auto_statement_alloc)
};

} // anonymous namespace

ref_counted_statement_base::ref_counted_statement_base(session& s)
    : refCount_(1), session_(s), need_comma_(false)
{
}

void ref_counted_statement::final_action()
{
    auto_statement_alloc auto_st_alloc(st_);

    st_.prepare(session_.get_query(), st_one_time_query);
    st_.define_and_bind();
    st_.execute(true);
}

std::ostringstream& ref_counted_statement_base::get_query_stream()
{
    return session_.get_query_stream();
}
