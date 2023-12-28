//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BOOST_OPTIONAL_H_INCLUDED
#define SOCI_BOOST_OPTIONAL_H_INCLUDED

#include "soci/type-conversion-traits.h"
// boost
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

namespace soci
{

// simple fall-back for boost::optional
template <typename T>
struct type_conversion<boost::optional<T> >
{
    typedef typename type_conversion<T>::base_type base_type;

    struct from_base_check : std::integral_constant<bool, true> {};

    static void from_base(base_type const & in, indicator ind,
        boost::optional<T> & out)
    {
        if (ind == i_null)
        {
            out.reset();
        }
        else
        {
            T tmp = T();
            type_conversion<T>::from_base(in, ind, tmp);
            out = tmp;
        }
    }

    struct move_from_base_check :
        std::integral_constant<bool,
            !std::is_const<base_type>::value
            && std::is_constructible<boost::optional<T>, typename std::add_rvalue_reference<base_type>::type>::value
        > {};


    static void move_from_base(base_type & in, indicator ind, boost::optional<T> & out)
    {
        static_assert(move_from_base_check::value,
                "move_to_base can only be used if the target type can be constructed from an rvalue base reference");
        if (ind == i_null)
        {
            out.reset();
        }
        else
        {
            out = std::move(in);
        }
    }

    static void to_base(boost::optional<T> const & in,
        base_type & out, indicator & ind)
    {
        if (in.is_initialized())
        {
            type_conversion<T>::to_base(in.get(), out, ind);
        }
        else
        {
            ind = i_null;
        }
    }

    static void move_to_base(boost::optional<T> & in, base_type & out, indicator & ind)
    {
        if (in.is_initialized())
        {
            out = std::move(in.get());
            ind = i_ok;
        }
        else
        {
            ind = i_null;
        }
    }
};

} // namespace soci

#endif // SOCI_BOOST_OPTIONAL_H_INCLUDED
