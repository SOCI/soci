//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci-mysql.h"
#include "common.h"
#include <soci.h>
#include <soci-platform.h>
#include <ciso646>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::mysql;


void mysql_vector_into_type_backend::define_by_pos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void mysql_vector_into_type_backend::pre_fetch()
{
    // nothing to do here
}

namespace // anonymous
{

template <typename T, typename U>
void set_invector_(void *p, int indx, U const &val)
{
    std::vector<T> *dest =
        static_cast<std::vector<T> *>(p);

    std::vector<T> &v = *dest;
    v[indx] = val;
}

} // namespace anonymous

void mysql_vector_into_type_backend::post_fetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // Here, rowsToConsume_ in the Statement object designates
        // the number of rows that need to be put in the user's buffers.

        // PostgreSQL column positions start at 0
        int pos = position_ - 1;

        int const endRow = statement_.currentRow_ + statement_.rowsToConsume_;
	
        mysql_data_seek(statement_.result_, statement_.currentRow_);
        for (int curRow = statement_.currentRow_, i = 0;
             curRow != endRow; ++curRow, ++i)
        {
            MYSQL_ROW row = mysql_fetch_row(statement_.result_);
            // first, deal with indicators
            if (row[pos] == NULL)
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

            // buffer with data retrieved from server, in text format
            const char *buf = row[pos] != NULL ? row[pos] : "";

            switch (type_)
            {
            case eXChar:
                set_invector_<char>(data_, i, *buf);
                break;
            case eXStdString:
                set_invector_<std::string>(data_, i, buf);
                break;
            case eXShort:
                {
                    long val = strtol(buf, NULL, 10);
                    set_invector_<short>(data_, i, static_cast<short>(val));
                }
                break;
            case eXInteger:
                {
                    long val = strtol(buf, NULL, 10);
                    set_invector_<int>(data_, i, static_cast<int>(val));
                }
                break;
            case eXUnsignedLong:
                {
                    long long val = strtoll(buf, NULL, 10);
                    set_invector_<unsigned long>(data_, i,
                        static_cast<unsigned long>(val));
                }
                break;
            case eXDouble:
                {
                    double val = strtod(buf, NULL);
                    set_invector_<double>(data_, i, val);
                }
                break;
            case eXStdTm:
                {
                    // attempt to parse the string and convert to std::tm
                    std::tm t;
                    parse_std_tm(buf, t);

                    set_invector_<std::tm>(data_, i, t);
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

namespace // anonymous
{

template <typename T>
void resizevector_(void *p, std::size_t sz)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

} // namespace anonymous

void mysql_vector_into_type_backend::resize(std::size_t sz)
{
    switch (type_)
    {
        // simple cases
    case eXChar:         resizevector_<char>         (data_, sz); break;
    case eXShort:        resizevector_<short>        (data_, sz); break;
    case eXInteger:      resizevector_<int>          (data_, sz); break;
    case eXUnsignedLong: resizevector_<unsigned long>(data_, sz); break;
    case eXDouble:       resizevector_<double>       (data_, sz); break;
    case eXStdString:    resizevector_<std::string>  (data_, sz); break;
    case eXStdTm:        resizevector_<std::tm>      (data_, sz); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }
}

std::size_t mysql_vector_into_type_backend::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
        // simple cases
    case eXChar:         sz = get_vector_size<char>         (data_); break;
    case eXShort:        sz = get_vector_size<short>        (data_); break;
    case eXInteger:      sz = get_vector_size<int>          (data_); break;
    case eXUnsignedLong: sz = get_vector_size<unsigned long>(data_); break;
    case eXDouble:       sz = get_vector_size<double>       (data_); break;
    case eXStdString:    sz = get_vector_size<std::string>  (data_); break;
    case eXStdTm:        sz = get_vector_size<std::tm>      (data_); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void mysql_vector_into_type_backend::clean_up()
{
    // nothing to do here
}

