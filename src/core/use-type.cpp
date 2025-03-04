//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/soci-platform.h"
#include "soci/use-type.h"
#include "soci/statement.h"
#include "soci/soci-unicode.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include "soci-vector-helpers.h"

#include <cstdio>

using namespace soci;
using namespace soci::details;

namespace
{

// Helper returning pointer to a vector element at the given index.
//
// This is only used in this file currently but could be extracted into
// soci-vector-helpers.h if it turns out to be useful elsewhere.

void* get_vector_element(exchange_type e, void* data, int index)
{
    switch (e)
    {
        case x_char:
            return &exchange_vector_type_cast<x_char>(data).at(index);
        case x_stdstring:
            return &exchange_vector_type_cast<x_stdstring>(data).at(index);
        case x_stdwstring:
            return &exchange_vector_type_cast<x_stdwstring>(data).at(index);
        case x_int8:
            return &exchange_vector_type_cast<x_int8>(data).at(index);
        case x_uint8:
            return &exchange_vector_type_cast<x_uint8>(data).at(index);
        case x_int16:
            return &exchange_vector_type_cast<x_int16>(data).at(index);
        case x_uint16:
            return &exchange_vector_type_cast<x_uint16>(data).at(index);
        case x_int32:
            return &exchange_vector_type_cast<x_int32>(data).at(index);
        case x_uint32:
            return &exchange_vector_type_cast<x_uint32>(data).at(index);
        case x_int64:
            return &exchange_vector_type_cast<x_int64>(data).at(index);
        case x_uint64:
            return &exchange_vector_type_cast<x_uint64>(data).at(index);
        case x_double:
            return &exchange_vector_type_cast<x_double>(data).at(index);
        case x_stdtm:
            return &exchange_vector_type_cast<x_stdtm>(data).at(index);
        case x_xmltype:
            return &exchange_vector_type_cast<x_xmltype>(data).at(index);
        case x_longstring:
            return &exchange_vector_type_cast<x_longstring>(data).at(index);
        case x_statement:
            // There is no vector of statements, but this is fine because we
            // don't use this value in do_dump_value anyhow, so we may just
            // return null.
            return NULL;
        case x_rowid:
            // Same as for x_statement above.
            return NULL;
        case x_blob:
            return &exchange_vector_type_cast<x_blob>(data).at(index);
    }

    return NULL;
}

// Common part of scalar and vector use types.
void
do_dump_value(std::ostream& os,
              exchange_type type,
              void* data,
              indicator const* ind)
{
    if (ind && *ind == i_null)
    {
        os << "NULL";
        return;
    }

    switch (type)
    {
        case x_char:
            os << "'" << exchange_type_cast<x_char>(data) << "'";
            return;

        case x_stdstring:
            // TODO: Escape quotes?
            os << "\"" << exchange_type_cast<x_stdstring>(data) << "\"";
            return;

        case x_stdwstring:
            os << "\"" << wide_to_utf8(exchange_type_cast<x_stdwstring>(data)) << "\"";
            return;

        case x_int8:
            os << exchange_type_cast<x_int8>(data);
            return;

        case x_uint8:
            os << exchange_type_cast<x_uint8>(data);
            return;

        case x_int16:
            os << exchange_type_cast<x_int16>(data);
            return;

        case x_uint16:
            os << exchange_type_cast<x_uint16>(data);
            return;

        case x_int32:
            os << exchange_type_cast<x_int32>(data);
            return;

        case x_uint32:
            os << exchange_type_cast<x_uint32>(data);
            return;

        case x_int64:
            os << exchange_type_cast<x_int64>(data);
            return;

        case x_uint64:
            os << exchange_type_cast<x_uint64>(data);
            return;

        case x_double:
            os << exchange_type_cast<x_double>(data);
            return;

        case x_stdtm:
            {
                std::tm const& t = exchange_type_cast<x_stdtm>(data);

                char buf[80];
                format_std_tm(t, buf, sizeof(buf));

                os << buf;
            }
            return;

        case x_statement:
            os << "<statement>";
            return;

        case x_rowid:
            os << "<rowid>";
            return;

        case x_blob:
            os << "<blob>";
            return;

        case x_xmltype:
            os << "<xml>";
            return;

        case x_longstring:
            os << "<long string>";
            return;
    }

    // This is normally unreachable, but avoid throwing from here as we're
    // typically called from an exception handler.
    os << "<unknown>";
}

} // anonymous namespace

standard_use_type::~standard_use_type()
{
    delete backEnd_;
}

void standard_use_type::bind(statement_impl & st, int & position)
{
    if (backEnd_ == NULL)
    {
        backEnd_ = st.make_use_type_backend();
    }

    if (name_.empty())
    {
        backEnd_->bind_by_pos(position, data_, type_, readOnly_);
    }
    else
    {
        backEnd_->bind_by_name(name_, data_, type_, readOnly_);
    }
}

void standard_use_type::dump_value(std::ostream& os, int /* index */) const
{
    do_dump_value(os, type_, data_, ind_);
}

void standard_use_type::pre_exec(int num)
{
    backEnd_->pre_exec(num);
}

void standard_use_type::pre_use()
{
    // Handle IN direction of parameters of SQL statements and procedures
    convert_to_base();
    backEnd_->pre_use(ind_);
}

void standard_use_type::post_use(bool gotData)
{
    // Handle OUT direction of IN/OUT parameters of stored procedures
    backEnd_->post_use(gotData, ind_);
    convert_from_base();

    // IMPORTANT:
    // This treatment of input ("use") parameter as output data sink may be
    // confusing, but it is necessary to store OUT data back in the same
    // object as IN, of IN/OUT parameter.
    // As there is no symmetry for IN/OUT in SQL and there are no OUT/IN
    // we do not perform convert_to_base() for output ("into") parameter.
    // See conversion_use_type<T>::convert_from_base() for more details.
}

void standard_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}

vector_use_type::~vector_use_type()
{
    delete backEnd_;
}

void vector_use_type::bind(statement_impl & st, int & position)
{
    if (backEnd_ == NULL)
    {
        backEnd_ = st.make_vector_use_type_backend();
    }

    if (name_.empty())
    {
        if (end_ != NULL)
        {
            backEnd_->bind_by_pos_bulk(position, data_, type_, begin_, end_);
        }
        else
        {
            backEnd_->bind_by_pos(position, data_, type_);
        }
    }
    else
    {
        if (end_ != NULL)
        {
            backEnd_->bind_by_name_bulk(name_, data_, type_, begin_, end_);
        }
        else
        {
            backEnd_->bind_by_name(name_, data_, type_);
        }
    }
}

void vector_use_type::dump_value(std::ostream& os, int index) const
{
    if (index != -1)
    {
        do_dump_value(
            os,
            type_,
            get_vector_element(type_, data_, index),
            ind_ ? &ind_->at(index) : NULL
        );
    }
    else
    {
        // We can't dump the whole vector, which could be huge, and it would be
        // pretty useless to do it anyhow as it still wouldn't give any
        // information about which vector element corresponds to the row which
        // triggered the error.
        os << "<vector>";
    }
}

void vector_use_type::pre_exec(int num)
{
    backEnd_->pre_exec(num);
}

void vector_use_type::pre_use()
{
    convert_to_base();

    backEnd_->pre_use(ind_ ? &ind_->at(0) : NULL);
}

std::size_t vector_use_type::size() const
{
    return backEnd_->size();
}

void vector_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}
