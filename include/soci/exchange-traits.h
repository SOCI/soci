//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_EXCHANGE_TRAITS_H_INCLUDED
#define SOCI_EXCHANGE_TRAITS_H_INCLUDED

#include "soci/soci-types.h"
#include "soci/type-conversion-traits.h"
#include "soci/soci-backend.h"
#include "soci/type-wrappers.h"
// std
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace soci
{

namespace details
{

struct basic_type_tag {};
struct user_type_tag {};

template <typename T>
struct exchange_traits
{
    // this is used for tag-dispatch between implementations for basic types
    // and user-defined types
    typedef user_type_tag type_family;

    enum // anonymous
    {
        x_type =
            exchange_traits
            <
                typename type_conversion<T>::base_type
            >::x_type
    };
};

template <>
struct exchange_traits<soci_int8_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_int8 };
};

template <>
struct exchange_traits<uint8_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_uint8 };
};

template <>
struct exchange_traits<int16_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_int16 };
};

template <>
struct exchange_traits<uint16_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_uint16 };
};

template <>
struct exchange_traits<int32_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_int32 };
};

template <>
struct exchange_traits<uint32_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_uint32 };
};

template <>
struct exchange_traits<int64_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_int64 };
};

template <>
struct exchange_traits<uint64_t>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_uint64 };
};

template <>
struct exchange_traits<soci_l_or_ll_t> : exchange_traits<soci_l_or_ll_int_t>
{
};

template <>
struct exchange_traits<soci_ul_or_ull_t> : exchange_traits<soci_ul_or_ull_int_t>
{
};

template <>
struct exchange_traits<double>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_double };
};

template <>
struct exchange_traits<char>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_char };
};

template <>
struct exchange_traits<std::string>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_stdstring };
};

template <>
struct exchange_traits<std::wstring>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_stdwstring };
};

template <>
struct exchange_traits<std::tm>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_stdtm };
};

template <typename T>
struct exchange_traits<std::vector<T> >
{
    typedef typename exchange_traits<T>::type_family type_family;
    enum { x_type = exchange_traits<T>::x_type };
};

// handling of wrapper types

template <>
struct exchange_traits<xml_type>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_xmltype };
};

template <>
struct exchange_traits<long_string>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_longstring };
};

} // namespace details

} // namespace soci

#endif // SOCI_EXCHANGE_TRAITS_H_INCLUDED
