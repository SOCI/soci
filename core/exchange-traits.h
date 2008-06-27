//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_EXCHANGE_TRAITS_H_INCLUDED
#define SOCI_EXCHANGE_TRAITS_H_INCLUDED

#include "type-conversion-traits.h"
#include "soci-backend.h"
#include <ctime>

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
        exchange_traits<typename type_conversion<T>::base_type>::x_type
    };
};

template <>
struct exchange_traits<short>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_short };
};

template <>
struct exchange_traits<int>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_integer };
};

template <>
struct exchange_traits<char>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_char };
};

template <>
struct exchange_traits<unsigned long>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_unsigned_long };
};

template <>
struct exchange_traits<long long>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_long_long };
};

template <>
struct exchange_traits<double>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_double };
};

template <>
struct exchange_traits<char *>
{
    typedef basic_type_tag type_family;
};

template <std::size_t N>
struct exchange_traits<char[N]>
{
    typedef basic_type_tag type_family;
};

template <>
struct exchange_traits<std::string>
{
    typedef basic_type_tag type_family;
    enum { x_type = x_stdstring };
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

} // namespace details

} // namespace soci

#endif // SOCI_EXCHANGE_TRAITS_H_INCLUDED
