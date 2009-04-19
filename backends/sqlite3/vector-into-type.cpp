//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <soci-platform.h>
#include "soci-sqlite3.h"
#include "common.h"
// std
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace soci;
using namespace soci::details;
using namespace soci::details::sqlite3;

void sqlite3_vector_into_type_backend::define_by_pos(int & position, void * data,
                                               exchange_type type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void sqlite3_vector_into_type_backend::pre_fetch()
{
    // ...
}

namespace // anonymous
{

template <typename T>
void setInVector(void *p, int indx, T const &val)
{
    std::vector<T> *dest =
    static_cast<std::vector<T> *>(p);

    std::vector<T> &v = *dest;
    v[indx] = val;
}

} // namespace anonymous

void sqlite3_vector_into_type_backend::post_fetch(bool gotData, indicator * ind)
{
    if (gotData)
    {
        int endRow = static_cast<int>(statement_.dataCache_.size());
        for (int i = 0; i < endRow; ++i)
        {
            const sqlite3_column& curCol =
                statement_.dataCache_[i][position_-1];

            if (curCol.isNull_)
            {
                if (ind == NULL)
                {
                    throw soci_error(
                        "Null value fetched and no indicator defined.");
                }

                ind[i] = i_null;
            }
            else
            {
                if (ind != NULL)
                {
                    ind[i] = i_ok;
                }
            }

            const char * buf = curCol.data_.c_str();


            // set buf to a null string if a null pointer is returned
            if (buf == NULL)
            {
                buf = "";
            }

            switch (type_)
            {
            case x_char:
                setInVector(data_, i, *buf);
                break;
            case x_stdstring:
                setInVector<std::string>(data_, i, buf);
                break;
            case x_short:
            {
                long val = std::strtol(buf, NULL, 10);
                setInVector(data_, i, static_cast<short>(val));
            }
            break;
            case x_integer:
            {
                long val = std::strtol(buf, NULL, 10);
                setInVector(data_, i, static_cast<int>(val));
            }
            break;
            case x_unsigned_long:
            {
                long long val = strtoll(buf, NULL, 10);
                setInVector(data_, i, static_cast<unsigned long>(val));
            }
            break;
            case x_long_long:
            {
                long long val = strtoll(buf, NULL, 10);
                setInVector(data_, i, val);
            }
            break;
            case x_double:
            {
                double val = strtod(buf, NULL);
                setInVector(data_, i, val);
            }
            break;
            case x_stdtm:
            {
                // attempt to parse the string and convert to std::tm
                std::tm t;
                parseStdTm(buf, t);

                setInVector(data_, i, t);
            }
            break;

            default:
                throw soci_error("Into element used with non-supported type.");
            }
        }
    }
    else // no data retrieved
    {
        // nothing to do, into vectors are already truncated
    }
}

void sqlite3_vector_into_type_backend::resize(std::size_t sz)
{
    switch (type_)
    {
        // simple cases
    case x_char:         resizeVector<char>         (data_, sz); break;
    case x_short:        resizeVector<short>        (data_, sz); break;
    case x_integer:      resizeVector<int>          (data_, sz); break;
    case x_unsigned_long: resizeVector<unsigned long>(data_, sz); break;
    case x_long_long:     resizeVector<long long>    (data_, sz); break;
    case x_double:       resizeVector<double>       (data_, sz); break;
    case x_stdstring:    resizeVector<std::string>  (data_, sz); break;
    case x_stdtm:        resizeVector<std::tm>      (data_, sz); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }
}

std::size_t sqlite3_vector_into_type_backend::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
        // simple cases
    case x_char:         sz = getVectorSize<char>         (data_); break;
    case x_short:        sz = getVectorSize<short>        (data_); break;
    case x_integer:      sz = getVectorSize<int>          (data_); break;
    case x_unsigned_long: sz = getVectorSize<unsigned long>(data_); break;
    case x_long_long:     sz = getVectorSize<long long>    (data_); break;
    case x_double:       sz = getVectorSize<double>       (data_); break;
    case x_stdstring:    sz = getVectorSize<std::string>  (data_); break;
    case x_stdtm:        sz = getVectorSize<std::tm>      (data_); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void sqlite3_vector_into_type_backend::clean_up()
{
    // ...
}
