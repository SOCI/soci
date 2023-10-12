//
// Copyright (C) 2010 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_DATETIME_TYPES_H_INCLUDED
#define SOCI_DATETIME_TYPES_H_INCLUDED

#include "soci/type-conversion-traits.h"
#include <limits>

namespace soci
{

SOCI_DECL std::tm to_std_tm ( const soci::datetime& dtm );
SOCI_DECL soci::datetime from_std_tm ( const std::tm& tm );

template <>
struct type_conversion<std::tm>
{
    typedef soci::datetime base_type;

    static void from_base(base_type const & in, indicator ind, std::tm& out )
    {
        if (ind == i_null)
        {
            return;
        }

        out = to_std_tm ( in );
    }

    static void to_base ( std::tm const& in,
        base_type & out, indicator & ind)
    {
        out = from_std_tm ( in );
        ind = i_ok;
    }
};

template <>
struct type_conversion<soci::datetime>
{
    typedef std::tm base_type;

    static void from_base ( base_type const& in, indicator ind, soci::datetime& out )
    {
        if ( ind == i_null )
        {
            return;
        }

        out = from_std_tm ( in );
    }

    static void to_base ( soci::datetime const& in, base_type& out, indicator& ind )
    {
        out = to_std_tm ( in );
        ind = i_ok;
    }
};

} // namespace soci

#endif // SOCI_DATETIME_TYPES_H_INCLUDED
