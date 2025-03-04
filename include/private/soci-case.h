//
// Copyright (C) 2024 Vadim Zeitlin.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_CASE_H_INCLUDED
#define SOCI_PRIVATE_SOCI_CASE_H_INCLUDED

#include <cctype>
#include <string>

namespace soci
{

namespace details
{

// Simplistic conversions of strings to upper/lower case.
//
// This doesn't work correctly for arbitrary Unicode strings for well-known
// reasons (such conversions can't be done correctly on char by char basis),
// but they do work for ASCII strings that we deal with and for anything else
// we'd need ICU -- which we could start using later, if necessary, by just
// replacing these functions with the versions using ICU functions instead.

inline std::string string_toupper(std::string const& s)
{
    std::string res;
    res.reserve(s.size());

    for (char c : s)
    {
        res += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    return res;
}

inline std::string string_tolower(std::string const& s)
{
    std::string res;
    res.reserve(s.size());

    for (char c : s)
    {
        res += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return res;
}

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_CASE_H_INCLUDED
