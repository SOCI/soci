//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2017 Vadim Zeitlin.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/error.h"
#include "soci-mktime.h"
#include <climits>
#include <cstdlib>
#include <ctime>

namespace // anonymous
{

// helper function for parsing decimal data (for std::tm)
int parse10(char const * & p1, char * & p2)
{
    long v = std::strtol(p1, &p2, 10);
    if (p2 != p1)
    {
        if (v < 0)
            throw soci::soci_error("Negative date/time field component.");

        if (v > INT_MAX)
            throw soci::soci_error("Out of range date/time field component.");

        p1 = p2 + 1;

        // Cast is safe due to check above.
        return static_cast<int>(v);
    }
    else
    {
        throw soci::soci_error("Cannot parse date/time field component.");

    }
}

} // namespace anonymous

void soci::details::parse_std_tm(char const * buf, std::tm & t)
{
    char const * p1 = buf;
    char * p2;
    char separator;
    int a, b, c;
    int year = 1900, month = 1, day = 1;
    int hour = 0, minute = 0, second = 0;

    a = parse10(p1, p2);
    separator = *p2;
    b = parse10(p1, p2);
    c = parse10(p1, p2);

    if (*p2 == ' ')
    {
        // there are more elements to parse
        // - assume that what was already parsed is a date part
        // and that the remaining elements describe the time of day
        year = a;
        month = b;
        day = c;
        hour   = parse10(p1, p2);
        minute = parse10(p1, p2);
        second = parse10(p1, p2);
    }
    else
    {
        // only three values have been parsed
        if (separator == '-')
        {
            // assume the date value was read
            // (leave the time of day as 00:00:00)
            year = a;
            month = b;
            day = c;
        }
        else
        {
            // assume the time of day was read
            // (leave the date part as 1900-01-01)
            hour = a;
            minute = b;
            second = c;
        }
    }

    mktime_from_ymdhms(t, year, month, day, hour, minute, second);
}

namespace
{
bool is_leap_year ( time_t y )
{
    return ( ( ( y % 4 == 0 ) && ( y % 100 != 0 ) ) || ( ( y + 1900 ) % 400 == 0 ) );
}

time_t elapsed_leap_years ( time_t y )
{
    return ( ( ( y - 1 ) / 4 ) - ( ( y - 1 ) / 100 ) + ( ( y + 299 ) / 400 ) - 17 );
}

}  // namespace

time_t soci::details::timegm_impl_soci ( struct tm* tb )
{
    static int days_by_month[] = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};

    time_t    tmptm1, tmptm2, tmptm3;

    if ( tb == NULL )
    {
        return static_cast<time_t> ( -1 );
    }

    tmptm1 = tb->tm_year;

    /*
     * Adjust month value so it is in the range 0 - 11.  This is because
     * we don't know how many days are in months 12, 13, 14, etc.
     */

    if ( ( tb->tm_mon < 0 ) || ( tb->tm_mon > 11 ) )
    {
        tmptm1 += ( tb->tm_mon / 12 );

        if ( ( tb->tm_mon %= 12 ) < 0 )
        {
            tb->tm_mon += 12;
            tmptm1--;
        }
    }

    /***** HERE: tmptm1 holds number of elapsed years *****/

    /*
     * Calculate days elapsed minus one, in the given year, to the given
     * month. Check for leap year and adjust if necessary.
     */
    tmptm2 = days_by_month[tb->tm_mon];
    if ( is_leap_year ( tmptm1 ) && ( tb->tm_mon > 1 ) )
        tmptm2++;

    /*
     * Calculate elapsed days since base date (midnight, 1/1/70, UTC)
     *
     *
     * 365 days for each elapsed year since 1970, plus one more day for
     * each elapsed leap year. no danger of overflow because of the range
     * check (above) on tmptm1.
     */
    tmptm3 = ( tmptm1 - 70 ) * 365 + elapsed_leap_years ( tmptm1 );

    /*
     * elapsed days to current month (still no possible overflow)
     */
    tmptm3 += tmptm2;

    /*
     * elapsed days to current date.
     */
    tmptm1 = tmptm3 + ( tmptm2 = static_cast<time_t> ( tb->tm_mday ) );

    /***** HERE: tmptm1 holds number of elapsed days *****/

    /*
     * Calculate elapsed hours since base date
     */
    tmptm2 = tmptm1 * 24;

    tmptm1 = tmptm2 + ( tmptm3 = static_cast<time_t> ( tb->tm_hour ) );

    /***** HERE: tmptm1 holds number of elapsed hours *****/

    /*
     * Calculate elapsed minutes since base date
     */

    tmptm2 = tmptm1 * 60;

    tmptm1 = tmptm2 + ( tmptm3 = static_cast<time_t> ( tb->tm_min ) );

    /***** HERE: tmptm1 holds number of elapsed minutes *****/

    /*
     * Calculate elapsed seconds since base date
     */

    tmptm2 = tmptm1 * 60;

    tmptm1 = tmptm2 + ( tmptm3 = static_cast<time_t> ( tb->tm_sec ) );

    /***** HERE: tmptm1 holds number of elapsed seconds *****/

    return tmptm1;
}

