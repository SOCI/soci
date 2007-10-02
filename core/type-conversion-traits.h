//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TYPE_CONVERSION_TRAITS_H_INCLUDED
#define TYPE_CONVERSION_TRAITS_H_INCLUDED

#include "soci-backend.h"

namespace soci
{

// default traits class type_conversion, acts as pass through for Row::get()
// when no actual conversion is needed.
template <typename T>
struct type_conversion
{
    typedef T base_type;

    static void from_base(base_type const &in, eIndicator ind, T &out)
    {
        if (ind == eNull)
        {
            throw soci_error("Null value not allowed for this type");
        }
        out = in;
    }

    static void to_base(T const &in, base_type &out, eIndicator &ind)
    {
        out = in;
        ind = eOK;
    }
};

} // namespace soci

#endif // TYPE_CONVERSION_TRAITS_H_INCLUDED
