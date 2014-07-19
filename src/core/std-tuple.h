//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_STD_TUPLE_H_INCLUDED
#define SOCI_STD_TUPLE_H_INCLUDED

#include "values.h"
#include "type-conversion-traits.h"
// std
#include <tuple>

namespace soci
{

template <typename T0>
struct type_conversion<std::tuple<T0> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0> & out)
    {
        in
            >> std::get<0>(out);
    }

    static void to_base(std::tuple<T0> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in);
    }
};

template <typename T0, typename T1>
struct type_conversion<std::tuple<T0, T1> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out);
    }

    static void to_base(std::tuple<T0, T1> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in);
    }
};

template <typename T0, typename T1, typename T2>
struct type_conversion<std::tuple<T0, T1, T2> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out);
    }

    static void to_base(std::tuple<T0, T1, T2> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3>
struct type_conversion<std::tuple<T0, T1, T2, T3> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3, T4> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4, T5> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4, T5> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out)
            >> std::get<5>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3, T4, T5> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in)
            << std::get<5>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4, T5, T6> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4, T5, T6> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out)
            >> std::get<5>(out)
            >> std::get<6>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3, T4, T5, T6> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in)
            << std::get<5>(in)
            << std::get<6>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4, T5, T6, T7> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out)
            >> std::get<5>(out)
            >> std::get<6>(out)
            >> std::get<7>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3, T4, T5, T6, T7> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in)
            << std::get<5>(in)
            << std::get<6>(in)
            << std::get<7>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out)
            >> std::get<5>(out)
            >> std::get<6>(out)
            >> std::get<7>(out)
            >> std::get<8>(out);
    }

    static void to_base(std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in)
            << std::get<5>(in)
            << std::get<6>(in)
            << std::get<7>(in)
            << std::get<8>(in);
    }
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
struct type_conversion<std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator ind,
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> & out)
    {
        in
            >> std::get<0>(out)
            >> std::get<1>(out)
            >> std::get<2>(out)
            >> std::get<3>(out)
            >> std::get<4>(out)
            >> std::get<5>(out)
            >> std::get<6>(out)
            >> std::get<7>(out)
            >> std::get<8>(out)
            >> std::get<9>(out);
    }

    static void to_base(
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> & in,
        base_type & out, indicator & ind)
    {
        out
            << std::get<0>(in)
            << std::get<1>(in)
            << std::get<2>(in)
            << std::get<3>(in)
            << std::get<4>(in)
            << std::get<5>(in)
            << std::get<6>(in)
            << std::get<7>(in)
            << std::get<8>(in)
            << std::get<9>(in);
    }
};

} // namespace soci

#endif // SOCI_STD_TUPLE_H_INCLUDED
