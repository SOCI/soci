//
// Copyright (C) 2020 Vadim Zeitlin.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_CSTRTOI_H_INCLUDED
#define SOCI_PRIVATE_SOCI_CSTRTOI_H_INCLUDED

#include "soci/error.h"

#include <cstdlib>
#include <limits>

namespace soci
{

namespace details
{

// Convert string to a signed value of the given type, checking for overflow.
//
// Fill the provided result parameter and return true on success or false on
// error, e.g. if the string couldn't be converted at all, if anything remains
// in the string after conversion or if the value is out of range.
template <typename T>
bool cstring_to_integer(T& result, char const* buf)
{
    char * end;

    // No strtoll() on MSVC versions prior to Visual Studio 2013
#if !defined (_MSC_VER) || (_MSC_VER >= 1800)
    long long t = strtoll(buf, &end, 10);
#else
    long long t = _strtoi64(buf, &end, 10);
#endif

    if (end == buf || *end != '\0')
        return false;

    // successfully converted to long long
    // and no other characters were found in the buffer

    const T max = (std::numeric_limits<T>::max)();
    const T min = (std::numeric_limits<T>::min)();
    if (t > static_cast<long long>(max) || t < static_cast<long long>(min))
        return false;

    result = static_cast<T>(t);

    return true;
}

// Similar to the above, but for the unsigned integral types.
template <typename T>
bool cstring_to_unsigned(T& result, char const* buf)
{
    char * end;

    // No strtoll() on MSVC versions prior to Visual Studio 2013
#if !defined (_MSC_VER) || (_MSC_VER >= 1800)
    unsigned long long t = strtoull(buf, &end, 10);
#else
    unsigned long long t = _strtoui64(buf, &end, 10);
#endif

    if (end == buf || *end != '\0')
        return false;

    // successfully converted to unsigned long long
    // and no other characters were found in the buffer

    const T max = (std::numeric_limits<T>::max)();
    if (t > static_cast<unsigned long long>(max))
        return false;

    result = static_cast<T>(t);

    return true;
}

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_CSTRTOI_H_INCLUDED
