//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_POSTGRESQL_COMMON_H_INCLUDED
#define SOCI_POSTGRESQL_COMMON_H_INCLUDED

#include "soci/postgresql/soci-postgresql.h"
#include "soci-cstrtoi.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

namespace soci
{

namespace details
{

namespace postgresql
{

// helper function for parsing boolean values as integers, throws if parsing
// fails.
template <typename T>
T parse_as_boolean_or_throw(char const * buf)
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

template <typename T>
T string_to_integer(char const * buf)
{
    T result;
    if (!cstring_to_integer(result, buf))
        result = parse_as_boolean_or_throw<T>(buf);

    return result;
}

// helper function for parsing unsigned integers
template <typename T>
T string_to_unsigned_integer(char const * buf)
{
    T result;
    if (!cstring_to_unsigned(result, buf))
        result = parse_as_boolean_or_throw<T>(buf);

    return result;
}

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
