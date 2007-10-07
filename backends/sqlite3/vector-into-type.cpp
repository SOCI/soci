//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci-sqlite3.h"

#include "common.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#define strtoll(s, p, b) static_cast<long long>(_strtoi64(s, p, b))
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::sqlite3;


void sqlite3_vector_into_type_backend::define_by_pos(int & position, void * data,
                                               eExchangeType type)
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

template <typename T, typename U>
void setInVector(void *p, int indx, U const &val)
{
    std::vector<T> *dest =
    static_cast<std::vector<T> *>(p);

    std::vector<T> &v = *dest;
    v[indx] = val;
}

} // namespace anonymous

void sqlite3_vector_into_type_backend::post_fetch(bool gotData, eIndicator * ind)
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

                ind[i] = eNull;
            }
            else
            {
                if (ind != NULL)
                {
                    ind[i] = eOK;
                }
            }

            const char * buf = curCol.data_.c_str();


            // set buf to a null string if a null pointer is returned
            if (!buf)
            {
                buf = "";
            }

            switch (type_)
            {
            case eXChar:
                setInVector<char>(data_, i, *buf);
                break;
            case eXStdString:
                setInVector<std::string>(data_, i, buf);
                break;
            case eXShort:
            {
                long val = strtol(buf, NULL, 10);
                setInVector<short>(data_, i, static_cast<short>(val));
            }
            break;
            case eXInteger:
            {
                long val = strtol(buf, NULL, 10);
                setInVector<int>(data_, i, static_cast<int>(val));
            }
            break;
            case eXUnsignedLong:
            {
                long long val = strtoll(buf, NULL, 10);
                setInVector<unsigned long>(data_, i,
                                           static_cast<unsigned long>(val));
            }
            break;
            case eXDouble:
            {
                double val = strtod(buf, NULL);
                setInVector<double>(data_, i, val);
            }
            break;
            case eXStdTm:
            {
                // attempt to parse the string and convert to std::tm
                std::tm t;
                parseStdTm(buf, t);

                setInVector<std::tm>(data_, i, t);
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
    case eXChar:         resizeVector<char>         (data_, sz); break;
    case eXShort:        resizeVector<short>        (data_, sz); break;
    case eXInteger:      resizeVector<int>          (data_, sz); break;
    case eXUnsignedLong: resizeVector<unsigned long>(data_, sz); break;
    case eXDouble:       resizeVector<double>       (data_, sz); break;
    case eXStdString:    resizeVector<std::string>  (data_, sz); break;
    case eXStdTm:        resizeVector<std::tm>      (data_, sz); break;

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
    case eXChar:         sz = getVectorSize<char>         (data_); break;
    case eXShort:        sz = getVectorSize<short>        (data_); break;
    case eXInteger:      sz = getVectorSize<int>          (data_); break;
    case eXUnsignedLong: sz = getVectorSize<unsigned long>(data_); break;
    case eXDouble:       sz = getVectorSize<double>       (data_); break;
    case eXStdString:    sz = getVectorSize<std::string>  (data_); break;
    case eXStdTm:        sz = getVectorSize<std::tm>      (data_); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void sqlite3_vector_into_type_backend::clean_up()
{
    // ...
}
