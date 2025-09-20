//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
#define SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED

#include "soci-compiler.h"

#include <climits>
#include <stdexcept>

// Many headers define std::ssize(), include this one because it's the most
// commonly included ones, so it doesn't cost much to include it even when not
// using C++20.
#include <vector>

namespace soci
{

#if SOCI_CHECK_CXX_STD(202002L)

using std::ssize;

#else // !C++20

#include <type_traits>

using ssize_t = std::make_signed<std::size_t>::type;

// Provide simple implementation of C++20 std::ssize() sufficient for our needs
// ourselves.

template <typename C>
constexpr ssize_t ssize(C const& c)
{
    return static_cast<ssize_t>(c.size());
}

template <typename T, int N>
constexpr ssize_t ssize(T (&)[N]) noexcept
{
    return N;
}

#endif // C++20/!C++20

/**
    Cast size_t to integer safely, raising exception if the value is out of
    range.
 */
#ifdef _MSC_VER
    #if _MSC_VER < 1910
        // MSVS 2015 can't compile "if" and "throw" in constexpr functions.
        #define SOCI_NO_ICAST_CONSTEXPR
    #endif
#endif

#ifndef SOCI_NO_ICAST_CONSTEXPR
constexpr
#endif
inline int icast(size_t n)
{
    if ( n >= INT_MAX )
    {
        // The error message here is poor, but this is never supposed to
        // happen, i.e. if there is any possibility of the input parameter
        // really being out of range of int type, caller should check for it on
        // its own and provide a better error.
        throw std::runtime_error("Invalid integer cast.");
    }

    return static_cast<int>(n);
}

// Define a function returning size as int: this should be safe as we should
// never have any collections of size greater than INT_MAX, but check for it
// just in case.
template <typename C>
constexpr inline int isize(C const& c)
{
    return icast(ssize(c));
}

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
