//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
#define SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED

#include "soci-compiler.h"

// Many headers define std::ssize(), include this one because it's the most
// commonly included ones, so it doesn't cost much to include it even when not
// using C++20.
#include <vector>

namespace soci
{

#if SOCI_CHECK_CXX_STD(202002L)

using std::ssize;

#else // !C++20

// Provide simple implementation of C++20 std::ssize() sufficient for our needs
// ourselves.

template <typename C>
constexpr int ssize(C const& c)
{
    return static_cast<int>(c.size());
}

#endif // C++20/!C++20

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
