//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#define SOCI_SOURCE
#include "row.h"
#include "statement.h"

#include <sstream>

using namespace soci;
using namespace details;

void row::add_properties(column_properties const &cp)
{
    columns_.push_back(cp);
    index_[cp.get_name()] = columns_.size() - 1;
}

std::size_t row::size() const
{
    return holders_.size();
}

eIndicator row::indicator(std::size_t pos) const
{
    assert(indicators_.size() >= static_cast<std::size_t>(pos + 1));
    return *indicators_[pos];
}

eIndicator row::indicator(std::string const &name) const
{
    return indicator(find_column(name));
}

column_properties const & row::get_properties(std::size_t pos) const
{
    assert(columns_.size() >= pos + 1);
    return columns_[pos];
}

column_properties const & row::get_properties(std::string const &name) const
{
    return get_properties(find_column(name));
}

std::size_t row::find_column(std::string const &name) const
{
    std::map<std::string, std::size_t>::const_iterator it = index_.find(name);
    if (it == index_.end())
    {
        std::ostringstream msg;
        msg << "Column '" << name << "' not found";
        throw soci_error(msg.str());
    }

    return it->second;
}

row::~row()
{
    std::size_t const hsize = holders_.size();
    for(std::size_t i = 0; i != hsize; ++i)
    {
        delete holders_[i];
        delete indicators_[i];
    }
}
