//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_STD_OPTIONAL_H_INCLUDED
#define SOCI_STD_OPTIONAL_H_INCLUDED

#include "soci/type-conversion-traits.h"

#include <optional>

namespace soci
{

// simple fall-back for std::optional
template <typename T>
struct type_conversion<std::optional<T> >
{
    typedef typename type_conversion<T>::base_type base_type;

    static void from_base(base_type const & in, indicator ind,
        std::optional<T> & out)
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

    static void to_base(std::optional<T> const & in,
        base_type & out, indicator & ind)
    {
        if (in)
        {
            type_conversion<T>::to_base(*in, out, ind);
        }
        else
        {
            ind = i_null;
        }
    }
};

} // namespace soci

#endif // SOCI_STD_OPTIONAL_H_INCLUDED
