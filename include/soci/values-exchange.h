//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_VALUES_EXCHANGE_H_INCLUDED
#define SOCI_VALUES_EXCHANGE_H_INCLUDED

#include "soci/values.h"
#include "soci/into-type.h"
#include "soci/use-type.h"
#include "soci/row-exchange.h"
// std
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace soci
{

namespace details
{

template <>
struct exchange_traits<values>
{
    typedef basic_type_tag type_family;

    // dummy value to satisfy the template engine, never used
    enum { x_type = 0 };
};

template <>
class use_type<values> : public use_type_base
{
public:
    use_type(values & v, std::string const & /*name*/ = std::string())
        : v_(v)
    {}

    // we ignore the possibility to have the whole values as NULL
    use_type(values & v, indicator /*ind*/, std::string const & /*name*/ = std::string())
        : v_(v)
    {}

    void bind(details::statement_impl & st, int & /*position*/) override
    {
        v_.uppercase_column_names(st.session_.get_uppercase_column_names());

        convert_to_base();
        st.bind(v_);
    }

    std::string get_name() const override
    {
        std::ostringstream oss;

        oss << "(";

        std::size_t const num_columns = v_.get_number_of_columns();
        for (std::size_t n = 0; n < num_columns; ++n)
        {
            if (n != 0)
                oss << ", ";

            oss << v_.get_properties(n).get_name();
        }

        oss << ")";

        return oss.str();
    }

    void dump_value(std::ostream& os) const override
    {
        // TODO: Dump all columns.
        os << "<value>";
    }

    void pre_exec(int /* num */) override {}

    void post_use(bool /*gotData*/) override
    {
        v_.reset_get_counter();
        convert_from_base();
    }

    void pre_use() override {convert_to_base();}
    void clean_up() override {v_.clean_up();}
    std::size_t size() const override { return 1; }

    // these are used only to re-dispatch to derived class
    // (the derived class might be generated automatically by
    // user conversions)
    virtual void convert_to_base() {}
    virtual void convert_from_base() {}

private:
    values & v_;

    SOCI_NOT_COPYABLE(use_type)
};

// this is not supposed to be used - no support for bulk ORM
template <>
class use_type<std::vector<values> >
{
private:
    use_type();
};

template <>
class into_type<values> : public into_type<row>
{
public:
    into_type(values & v, std::size_t bulk_size = 1)
        : into_type<row>(v.get_row()), v_(v)
    {
        v.row_->bulk_size(bulk_size);
    }

    into_type(values & v, indicator & ind, std::size_t bulk_size = 1)
        : into_type<row>(v.get_row(), ind), v_(v)
    {
        v.row_->bulk_size(bulk_size);
    }

    void clean_up() override
    {
        v_.clean_up();
    }

private:
    values & v_;

    SOCI_NOT_COPYABLE(into_type)
};

// this is not supposed to be used - no support for bulk ORM
template <>
class into_type<std::vector<values> >
{
private:
    into_type();
};

//this is for support bulk ORM
template <typename T>
class conversion_into_type<std::vector<T>, values >
    :private base_value_holder<T>,  //val_ is values
     public into_type< values > 
{
public:
    typedef values base_type; 

    conversion_into_type(std::vector<T> & value)
        : into_type<base_type>(base_value_holder<T>::val_, value.size())
        , value_(value)
        //        , ind_(ownInd_)
    {}

    //ind is not used
    conversion_into_type(std::vector<T> & value, std::vector<indicator> &)
        : into_type<base_type>(base_value_holder<T>::val_, value.size())
        , value_(value)
    {}

    virtual std::size_t size() const
    {
        // the user might have resized his vector in the meantime
        // -> synchronize the base-value mirror to have the same size

        return value_.size();
    }

    virtual void resize(std::size_t sz)
    {
        value_.resize(sz);
        //ind_.resize(sz);
    }

private:
    void convert_from_base()
    {
        values& v = base_value_holder<T>::val_;
        std::size_t data_size = value_.size();

        for (std::size_t i=0; i<data_size; ++i)
        {
            type_conversion<T>::from_base(v, i_ok, value_[i]);

            v.row_->next();
        }
    }

    std::vector<T> & value_;

    //do we need indicators for ORM? ... currently I don't know how to do this, so I just comment it ...

    //std::vector<indicator> ownInd_;

    // ind_ refers to either ownInd_, or the one provided by the user
    // in any case, ind_ refers to some valid vector of indicators
    // and can be used by conversion routines
    //std::vector<indicator> & ind_;
};

} // namespace details

} // namespace soci

#endif // SOCI_VALUES_EXCHANGE_H_INCLUDED
