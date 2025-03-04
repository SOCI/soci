//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/row.h"
#include "soci/type-holder.h"

#include <cstddef>
#include <sstream>
#include <string>

#include "soci-case.h"

using namespace soci;
using namespace details;

row::row()
    : uppercaseColumnNames_(false)
    , currentPos_(0)
{}

row::~row()
{
    clean_up();
}

void row::uppercase_column_names(bool forceToUpper)
{
    uppercaseColumnNames_ = forceToUpper;
}

void row::add_properties(column_properties const &cp)
{
    columns_.push_back(cp);

    std::string columnName;
    std::string const & originalName = cp.get_name();
    if (uppercaseColumnNames_)
    {
        columnName = string_toupper(originalName);

        // rewrite the column name in the column_properties object
        // as well to retain consistent views

        columns_[columns_.size() - 1].set_name(columnName);
    }
    else
    {
        columnName = originalName;
    }

    index_[columnName] = columns_.size() - 1;
}

std::size_t row::size() const
{
    return holders_.size();
}

void row::clean_up()
{
    std::size_t const hsize = holders_.size();
    for (std::size_t i = 0; i != hsize; ++i)
    {
        delete holders_[i];
        delete indicators_[i];
    }

    columns_.clear();
    holders_.clear();
    indicators_.clear();
    index_.clear();
}

indicator row::get_indicator(std::size_t pos) const
{
    return *indicators_.at(pos);
}

indicator row::get_indicator(std::string const &name) const
{
    return get_indicator(find_column(name));
}

column_properties const & row::get_properties(std::size_t pos) const
{
    return columns_.at(pos);
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

template <>
blob row::move_as<blob>(std::size_t pos) const
{
    typedef typename type_conversion<blob>::base_type base_type;
    base_type & baseVal = holders_.at(pos)->get<base_type>(value_reference_tag{});

    blob ret;
    type_conversion<blob>::move_from_base(baseVal, *indicators_.at(pos), ret);

    // Re-initialize blob object so it can be used in further queries
    baseVal.initialize(ret.get_backend()->get_session_backend().make_blob_backend());

    return ret;
}
