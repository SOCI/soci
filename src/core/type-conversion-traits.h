//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
#define SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED

#include "soci-backend.h"
#ifdef HAVE_BOOST
#include <string>
#include <boost/lexical_cast.hpp>
#endif

namespace soci
{

// default traits class type_conversion, acts as pass through for row::get()
// when no actual conversion is needed.
template <typename T, typename Enable = void>
struct type_conversion
{
    typedef T base_type;

    static void from_base(base_type const & in, indicator ind, T & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type");
        }
        out = in;
    }

    static void to_base(T const & in, base_type & out, indicator & ind)
    {
        out = in;
        ind = i_ok;
    }
};

#ifdef HAVE_BOOST
template <typename T>
struct string_conversion
{
    static void from_string(std::string const & in, indicator ind, T & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for string type");
        }
        throw std::bad_cast();
    }
};

template <>
struct string_conversion<double>
{
    static void from_string(std::string const & in, indicator ind, double & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for string type");
        }
        out = boost::lexical_cast<double>(in);
    }
};
#endif

} // namespace soci

#endif // SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
