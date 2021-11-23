//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/row.h"

#include <cstddef>
#include <cctype>
#include <sstream>
#include <string>

using namespace soci;
using namespace details;

namespace soci
{
    namespace details
    {
        class data_holder
        {
        public:
            explicit data_holder(data_type tp) : type(tp)
            {
                switch (type) {
                    case dt_string:
                        data.p = new std::string();
                        break;
                    case dt_date:
                        data.p = new std::tm();
                        break;
                    case dt_double:
                        data.d = 0;
                        break;
                    case dt_integer:
                        data.i = 0;
                        break;
                    case dt_long_long:
                        data.ll = 0;
                        break;
                    case dt_unsigned_long_long:
                        data.ull = 0;
                        break;
                    default:
                        break;
                }
                ind = i_ok;
            }
            ~data_holder()
            {
                switch (type)
                {
                    case dt_string:
                        delete (std::string *) data.p;
                        break;
                    case dt_date:
                        delete (std::tm *) data.p;
                        break;
                    default:
                        break;
                }
            }

        public:
            data_type type;
            union
            {
                void *p;
                double d;
                int i;
                long long ll;
                unsigned long long ull;
            } data;
            indicator ind;
        };
    } // namespace details
} // namespace soci

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
        for (std::size_t i = 0; i != originalName.size(); ++i)
        {
            columnName.push_back(static_cast<char>(std::toupper(originalName[i])));
        }

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
    for (std::vector<data_holder *>::iterator it = holders_.begin(); it != holders_.end(); ++it)
    {
        delete *it;
    }

    columns_.clear();
    holders_.clear();
    index_.clear();
}

indicator row::get_indicator(std::size_t pos) const
{
    return holders_.at(pos)->ind;
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

std::pair<std::string*, indicator*> row::alloc_data_holder_string()
{
    data_holder *holder = new data_holder(dt_string);
    holders_.push_back(holder);
    return std::make_pair((std::string *)holder->data.p, &holder->ind);
}

std::pair<std::tm*, indicator*> row::alloc_data_holder_tm()
{
    data_holder *holder = new data_holder(dt_date);
    holders_.push_back(holder);
    return std::make_pair((std::tm *)holder->data.p, &holder->ind);
}

std::pair<double*, indicator*> row::alloc_data_holder_double()
{
    data_holder *holder = new data_holder(dt_double);
    holders_.push_back(holder);
    return std::make_pair(&holder->data.d, &holder->ind);
}

std::pair<int*, indicator*> row::alloc_data_holder_int()
{
    data_holder *holder = new data_holder(dt_integer);
    holders_.push_back(holder);
    return std::make_pair(&holder->data.i, &holder->ind);
}

std::pair<long long*, indicator*> row::alloc_data_holder_llong()
{
    data_holder *holder = new data_holder(dt_long_long);
    holders_.push_back(holder);
    return std::make_pair(&holder->data.ll, &holder->ind);
}

std::pair<unsigned long long*, indicator*> row::alloc_data_holder_ullong()
{
    data_holder *holder = new data_holder(dt_unsigned_long_long);
    holders_.push_back(holder);
    return std::make_pair(&holder->data.ull, &holder->ind);
}

data_type row::get_data_holder_type(std::size_t pos) const SOCI_NOEXCEPT
{
    return holders_.at(pos)->type;
}

double row::get_data_holder_double(std::size_t pos) const SOCI_NOEXCEPT
{
    return holders_.at(pos)->data.d;
}

int row::get_data_holder_int(std::size_t pos) const SOCI_NOEXCEPT
{
    return holders_.at(pos)->data.i;
}

long long row::get_data_holder_llong(std::size_t pos) const SOCI_NOEXCEPT
{
    return holders_.at(pos)->data.ll;
}

unsigned long long row::get_data_holder_ullong(std::size_t pos) const SOCI_NOEXCEPT
{
    return holders_.at(pos)->data.ull;
}

void row::do_get(std::size_t pos, std::string &baseVal) const
{
    switch (holders_.at(pos)->type)
    {
        case dt_string:
        case dt_blob:
        case dt_xml:
            baseVal = *((std::string *)holders_[pos]->data.p);
            break;
        default:
            throw std::bad_cast();
    }
}

void row::do_get(std::size_t pos, std::tm &baseVal) const
{
    switch (holders_.at(pos)->type)
    {
        case dt_date:
            baseVal = *((std::tm *)holders_[pos]->data.p);
            break;
        default:
            throw std::bad_cast();
    }
}
