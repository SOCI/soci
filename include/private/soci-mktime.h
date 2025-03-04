//
// Copyright (C) 2015 Vadim Zeitlin.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_MKTIME_H_INCLUDED
#define SOCI_PRIVATE_SOCI_MKTIME_H_INCLUDED

// Not <ctime> because we also want to get timegm() if available.
#include <time.h>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace soci
{

namespace details
{

SOCI_DECL time_t timegm_impl_soci ( struct tm* tb );

template <typename T>
auto timegm_impl(T* t) -> decltype(timegm(t))
{
    return timegm(t);    
}

template <typename T>
auto timegm_impl(T t) -> time_t
{
    return timegm_impl_soci(t);
}

// Fill the provided struct tm with the values corresponding to the given date
// in UTC.
//
// Notice that both years and months are normal human 1-based values here and
// not 1900 or 0-based as in struct tm itself.
inline
void
mktime_from_ymdhms(tm& t,
                   int year, int month, int day,
                   int hour, int minute, int second)
{
    t.tm_isdst = -1;
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;

    timegm_impl(&t);
}

// Helper function for parsing datetime values.
//
// Throws if the string in buf couldn't be parsed as a date or a time string.
SOCI_DECL void parse_std_tm(char const *buf, std::tm &t);

// Reverse function for formatting datetime values.
//
// The string returned in the buffer uses YYYY-MM-DD HH:MM:SS format.
inline int format_std_tm(std::tm const& t, char* buf, std::size_t len)
{
    return snprintf(buf, len,
        "%d-%02d-%02d %02d:%02d:%02d",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);
}

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_MKTIME_H_INCLUDED
