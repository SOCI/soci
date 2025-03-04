//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
#define SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED

namespace soci
{

// Provide simple implementation of C++20 std::ssize().

template <typename C>
constexpr int ssize(C const& c)
{
    return static_cast<int>(c.size());
}

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_SSIZE_H_INCLUDED
