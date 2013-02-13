//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_FIREBIRD_COMMON_H_INCLUDED
#define SOCI_FIREBIRD_COMMON_H_INCLUDED

#include "soci-firebird.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace soci
{

namespace details
{

namespace firebird
{

char * allocBuffer(XSQLVAR* var);

void tmEncode(short type, std::tm * src, void * dst);

void tmDecode(short type, void * src, std::tm * dst);

void setTextParam(char const * s, std::size_t size, char * buf_,
    XSQLVAR * var);

std::string getTextParam(XSQLVAR const *var);

template <typename IntType>
const char *str2int(const char * s, IntType &out)
{
    int sign = 1;
    if ('+' == *s)
        ++s;
    else if ('-' == *s)
    {
        sign = -1;
        ++s;
    }
    IntType res = 0;
    for (out = 0; *s; ++s, out = res)
    {
        int d = *s - '0';
        if (d < 0 || d > 9)
            return s;
        res = res * 10 + d * sign;
        if (1 == sign)
        {
            if (res < out)
                return s;
        }
        else
        {
            if (res > out)
                return s;
        }
    }
    return s;
}

template<typename T1>
void to_isc(void * val, XSQLVAR * var)
{
    T1 value = *reinterpret_cast<T1*>(val);
    short scale = var->sqlscale;
    short type = var->sqltype & ~1;
    ISC_INT64 tens = 1;

    if ((std::numeric_limits<T1>::is_integer == false) && scale >= 0 &&
        (type == SQL_SHORT || type == SQL_LONG || type == SQL_INT64))
    {
        throw soci_error("Can't convert non-integral value to integral column type");
    }

    for (int i = 0; i > scale; --i)
    {
        tens *= 10;
    }

    switch (var->sqltype & ~1)
    {
    case SQL_SHORT:
        {
            short tmp = static_cast<short>(value*tens);
            std::memcpy(var->sqldata, &tmp, sizeof(short));
        }
        break;
    case SQL_LONG:
        {
            int tmp = static_cast<int>(value*tens);
            std::memcpy(var->sqldata, &tmp, sizeof(int));
        }
        break;
    case SQL_INT64:
        {
            long long tmp = static_cast<long long>(value*tens);
            std::memcpy(var->sqldata, &tmp, sizeof(long long));
        }
        break;
    case SQL_FLOAT:
        {
            float sql_value = static_cast<float>(value);
            std::memcpy(var->sqldata, &sql_value, sizeof(float));
        }
        break;
    case SQL_DOUBLE:
        {
            double sql_value = static_cast<double>(value);
            std::memcpy(var->sqldata, &sql_value, sizeof(double));
        }
        break;
    default:
        throw soci_error("Incorrect data type for numeric conversion");
    }
}

template<typename T1>
T1 from_isc(XSQLVAR * var)
{
    short scale = var->sqlscale;
    T1 tens = 1;

    if (scale < 0)
    {
        if (std::numeric_limits<T1>::is_integer)
        {
            std::ostringstream msg;
            msg << "Can't convert value with scale " << -scale
                << " to integral type";
            throw soci_error(msg.str());
        }

        for (int i = 0; i > scale; --i)
        {
            tens *= 10;
        }
    }

    switch (var->sqltype & ~1)
    {
    case SQL_SHORT:
        return static_cast<T1>(*reinterpret_cast<short*>(var->sqldata)/tens);
    case SQL_LONG:
        return static_cast<T1>(*reinterpret_cast<unsigned*>(var->sqldata)/tens);
    case SQL_INT64:
        return static_cast<T1>(*reinterpret_cast<long long*>(var->sqldata)/tens);
    case SQL_FLOAT:
        return static_cast<T1>(*reinterpret_cast<float*>(var->sqldata));
    case SQL_DOUBLE:
        return static_cast<T1>(*reinterpret_cast<double*>(var->sqldata));
    default:
        throw soci_error("Incorrect data type for numeric conversion");
    }
}

template <typename T>
std::size_t getVectorSize(void *p)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    return v->size();
}

template <typename T>
void resizeVector(void *p, std::size_t sz)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

} // namespace firebird

} // namespace details

} // namespace soci

#endif // SOCI_FIREBIRD_COMMON_H_INCLUDED
