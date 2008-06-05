//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
#define SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED

#include "soci-backend.h"

namespace soci
{

// default traits class type_conversion, acts as pass through for row::get()
// when no actual conversion is needed.
template <typename T>
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

        // Note: if the T type does not provide any specific NULL semantics,
        // then the indicator is left untouched
        // - this is the case for fundamental types
    }
};

} // namespace soci

#endif // SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
