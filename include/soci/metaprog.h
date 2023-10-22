//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_METAPROG_H_INCLUDED
#define SOCI_METAPROG_H_INCLUDED
#include <cstdint>

namespace soci
{
#if __cplusplus < 201103L
// ENABLE_IF
template<bool, class T = void> struct enable_if {};
template<class T> struct enable_if<true, T> { typedef T type; };

// IS_INTEGRAL
template <class T, T v>
struct integral_constant
{
    typedef T value_type;
    typedef integral_constant<T, v> type;
    static const T value = v;
    operator T() { return value; }
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template<typename T> struct is_integral : public false_type {};
template<typename T> struct is_integral<const T> : public is_integral<T> {};
template<typename T> struct is_integral<volatile const T> : public is_integral<T> {};
template<typename T> struct is_integral<volatile T> : public is_integral<T> {};

template<> struct is_integral<unsigned char> : public true_type {};
template<> struct is_integral<unsigned short> : public true_type {};
template<> struct is_integral<unsigned int> : public true_type {};
template<> struct is_integral<unsigned long> : public true_type {};
template<> struct is_integral<unsigned long long> : public true_type {};

template<> struct is_integral<signed char> : public true_type {};
template<> struct is_integral<short> : public true_type {};
template<> struct is_integral<int> : public true_type {};
template<> struct is_integral<long> : public true_type {};
template<> struct is_integral<long long> : public true_type {};

template<> struct is_integral<char> : public true_type {};
template<> struct is_integral<bool> : public true_type {};

#ifdef __GNUC__
template<> struct is_integral<uint8_t> : public true_type {};
template<> struct is_integral<uint16_t> : public true_type {};
template<> struct is_integral<uint32_t> : public true_type {};
template<> struct is_integral<uint64_t> : public true_type {};

template<> struct is_integral<int8_t> : public true_type {};
template<> struct is_integral<int16_t> : public true_type {};
template<> struct is_integral<int32_t> : public true_type {};
template<> struct is_integral<int64_t> : public true_type {};
#endif

// IS_SAME
template<class T, class U> struct is_same : public soci::false_type {};
template<class T> struct is_same<T, T> : public soci::true_type {};
#else
template<bool B, class C=void>
using enable_if = typename std::enable_if<B, C>;
template<typename T, typename U>
using is_same = typename std::is_same<T, U>;
template<typename T>
using is_integral = typename std::is_integral<T>;
#endif
} // namespace soci

#endif // SOCI_METAPROG_H_INCLUDED
