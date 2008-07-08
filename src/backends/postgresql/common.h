//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_POSTGRESQL_COMMON_H_INCLUDED
#define SOCI_POSTGRESQL_COMMON_H_INCLUDED

#include "soci-postgresql.h"
#include <limits>
#include <cstring>
#include <ctime>

namespace soci
{

namespace details
{

namespace postgresql
{

// helper function for parsing integers
template <typename T>
T string_to_integer(char const * buf)
{
    long long t;
    int n;
    int const converted = sscanf(buf, "%lld%n", &t, &n);
    if (converted == 1 && static_cast<std::size_t>(n) == strlen(buf))
    {
        // successfully converted to long long
        // and no other characters were found in the buffer

        const T max = (std::numeric_limits<T>::max)();
        const T min = (std::numeric_limits<T>::min)();
        if (t <= static_cast<long long>(max) &&
            t >= static_cast<long long>(min))
        {
            return static_cast<T>(t);
        }
        else
        {
            // value of out target range
            throw soci_error("Cannot convert data.");
        }
    }
    else
    {
        // try additional conversion from boolean
        // (PostgreSQL gives 't' or 'f' for boolean results)

        if (buf[0] == 't' && buf[1] == '\0')
        {
            return static_cast<T>(1);
        }
        else if (buf[0] == 'f' && buf[1] == '\0')
        {
            return static_cast<T>(0);
        }
        else
        {
            throw soci_error("Cannot convert data.");
        }
    }
}

// helper function for parsing doubles
double string_to_double(char const * buf);

// helper function for parsing datetime values
void parse_std_tm(char const * buf, std::tm & t);

// helper for vector operations
template <typename T>
std::size_t get_vector_size(void * p)
{
    std::vector<T> * v = static_cast<std::vector<T> *>(p);
    return v->size();
}

} // namespace postgresql

} // namespace details

} // namespace soci

#endif // SOCI_POSTGRESQL_COMMON_H_INCLUDED
