//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VALUES_EXCHANGE_H_INCLUDED
#define VALUES_EXCHANGE_H_INCLUDED

#include "values.h"
#include "into-type.h"
#include "use-type.h"
#include "row-exchange.h"

namespace soci
{

namespace details
{

template <>
struct exchange_traits<values>
{
    typedef basic_type_tag type_family;

    // dummy value to satisfy the template engine, never used
    enum { eXType = 0 };
};

template <>
class use_type<values> : public use_type_base
{
public:
    use_type(values &v, std::string const & /* name */ = std::string())
        : v_(v) {}

    // we ignore the possibility to have the whole values as NULL
    use_type(values &v, eIndicator /* ind */, std::string const & /* name */ = std::string())
        : v_(v) {}

    virtual void bind(details::statement_impl &st, int& /*position*/)
    {
        convert_to_base();
        st.bind(v_);
    }

    virtual void post_use(bool /*gotData*/)
    {
        convert_from_base();
    }

    virtual void pre_use() {}
    virtual void clean_up() {v_.clean_up();}
    virtual std::size_t size() const { return 1; }

    // these are used only to re-dispatch to derived class
    // (the derived class might be generated automatically by
    // user conversions)
    virtual void convert_to_base() {}
    virtual void convert_from_base() {}

private:
    values& v_;
};

template <>
class into_type<values> : public into_type<row>
{
public:
    into_type(values &v) : into_type<row>(v.get_row()), v_(v) {}
    into_type(values &v, eIndicator &ind)
        : into_type<row>(v.get_row(), ind), v_(v) {}

    void clean_up()
    {
        v_.clean_up();
    }

private:
    values &v_;
};

} // namespace details

} // namespace soci

#endif // VALUES_EXCHANGE_H_INCLUDED
