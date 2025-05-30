//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/soci-platform.h"
#include "soci/postgresql/soci-postgresql.h"
#include "soci-cstrtod.h"
#include "soci-mktime.h"
#include "common.h"
#include "soci/type-wrappers.h"

#include <libpq-fe.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

using namespace soci;
using namespace soci::details;
using namespace soci::details::postgresql;


void postgresql_vector_into_type_backend::define_by_pos_bulk(
    int & position, void * data, exchange_type type,
    std::size_t begin, std::size_t * end)
{
    data_ = data;
    type_ = type;
    begin_ = begin;
    end_ = end;
    position_ = position++;

    end_var_ = full_size();
}

void postgresql_vector_into_type_backend::pre_fetch()
{
    // nothing to do here
}

namespace // anonymous
{

template <typename T>
void set_invector_(void * p, int indx, T const & val)
{
    std::vector<T> * dest =
        static_cast<std::vector<T> *>(p);

    std::vector<T> & v = *dest;
    v[indx] = val;
}

template <typename T, typename V>
void set_invector_wrappers_(void * p, int indx, V const & val)
{
    std::vector<T> * dest =
        static_cast<std::vector<T> *>(p);

    std::vector<T> & v = *dest;
    v[indx].value = val;
}

} // namespace anonymous

void postgresql_vector_into_type_backend::post_fetch(bool gotData, indicator * ind)
{
    if (gotData)
    {
        // Here, rowsToConsume_ in the Statement object designates
        // the number of rows that need to be put in the user's buffers.

        // postgresql_ column positions start at 0
        int const pos = position_ - 1;

        int const endRow = statement_.currentRow_ + statement_.rowsToConsume_;

        for (int curRow = statement_.currentRow_, i = static_cast<int>(begin_);
             curRow != endRow; ++curRow, ++i)
        {
            // first, deal with indicators
            if (PQgetisnull(statement_.result_, curRow, pos) != 0)
            {
                if (ind == NULL)
                {
                    throw soci_error(
                        "Null value fetched and no indicator defined.");
                }

                ind[i] = i_null;

                // no need to convert data if it is null, go to next row
                continue;
            }
            else
            {
                if (ind != NULL)
                {
                    ind[i] = i_ok;
                }
            }

            // buffer with data retrieved from server, in text format
            char * buf = PQgetvalue(statement_.result_, curRow, pos);

            switch (type_)
            {
            case x_char:
                set_invector_(data_, i, *buf);
                break;
            case x_stdstring:
                set_invector_<std::string>(data_, i, buf);
                break;
            case x_int8:
                {
                    int8_t const val = string_to_integer<int8_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_uint8:
                {
                    uint8_t const val = string_to_integer<uint8_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_int16:
                {
                    int16_t const val = string_to_integer<int16_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_uint16:
                {
                    uint16_t const val = string_to_integer<uint16_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_int32:
                {
                    int32_t const val = string_to_integer<int32_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_uint32:
                {
                    uint32_t const val = string_to_integer<uint32_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_int64:
                {
                    int64_t const val = string_to_integer<int64_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_uint64:
                {
                    uint64_t val =
                        string_to_unsigned_integer<uint64_t>(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_double:
                {
                    double const val = cstring_to_double(buf);
                    set_invector_(data_, i, val);
                }
                break;
            case x_stdtm:
                {
                    // attempt to parse the string and convert to std::tm
                    std::tm t = std::tm();
                    parse_std_tm(buf, t);

                    set_invector_(data_, i, t);
                }
                break;
            case x_xmltype:
                set_invector_wrappers_<xml_type, std::string>(data_, i, buf);
                break;
            case x_longstring:
                set_invector_wrappers_<long_string, std::string>(data_, i, buf);
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
void resizevector_(void * p, std::size_t sz)
{
    std::vector<T> * v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

} // namespace anonymous

void postgresql_vector_into_type_backend::resize(std::size_t sz)
{
    if (user_ranges_)
    {
        // resize only in terms of user-provided ranges (below)
    }
    else
    {
        switch (type_)
        {
            // simple cases
        case x_char:
            resizevector_<char>(data_, sz);
            break;
        case x_int8:
            resizevector_<int8_t>(data_, sz);
            break;
        case x_uint8:
            resizevector_<uint8_t>(data_, sz);
            break;
        case x_int16:
            resizevector_<int16_t>(data_, sz);
            break;
        case x_uint16:
            resizevector_<uint16_t>(data_, sz);
            break;
        case x_int32:
            resizevector_<int32_t>(data_, sz);
            break;
        case x_uint32:
            resizevector_<uint32_t>(data_, sz);
            break;
        case x_int64:
            resizevector_<int64_t>(data_, sz);
            break;
        case x_uint64:
            resizevector_<uint64_t>(data_, sz);
            break;
        case x_double:
            resizevector_<double>(data_, sz);
            break;
        case x_stdstring:
            resizevector_<std::string>(data_, sz);
            break;
        case x_stdtm:
            resizevector_<std::tm>(data_, sz);
            break;
        case x_xmltype:
            resizevector_<xml_type>(data_, sz);
            break;
        case x_longstring:
            resizevector_<long_string>(data_, sz);
            break;
        default:
            throw soci_error("Into vector element used with non-supported type.");
        }

        end_var_ = sz;
    }

    // resize ranges, either user-provided or internally managed
    *end_ = begin_ + sz;
}

std::size_t postgresql_vector_into_type_backend::size() const
{
    // as a special error-detection measure, check if the actual vector size
    // was changed since the original bind (when it was stored in end_var_):
    const std::size_t actual_size = full_size();
    if (actual_size != end_var_)
    {
        // ... and in that case return the actual size
        return actual_size;
    }

    if (end_ != NULL && *end_ != 0)
    {
        return *end_ - begin_;
    }
    else
    {
        return end_var_;
    }
}

std::size_t postgresql_vector_into_type_backend::full_size() const
{
    std::size_t sz SOCI_DUMMY_INIT(0);
    switch (type_)
    {
        // simple cases
    case x_char:
        sz = get_vector_size<char>(data_);
        break;
    case x_int8:
        sz = get_vector_size<int8_t>(data_);
        break;
    case x_uint8:
        sz = get_vector_size<uint8_t>(data_);
        break;
    case x_int16:
        sz = get_vector_size<int16_t>(data_);
        break;
    case x_uint16:
        sz = get_vector_size<uint16_t>(data_);
        break;
    case x_int32:
        sz = get_vector_size<int32_t>(data_);
        break;
    case x_uint32:
        sz = get_vector_size<uint32_t>(data_);
        break;
    case x_int64:
        sz = get_vector_size<int64_t>(data_);
        break;
    case x_uint64:
        sz = get_vector_size<int64_t>(data_);
        break;
    case x_double:
        sz = get_vector_size<double>(data_);
        break;
    case x_stdstring:
        sz = get_vector_size<std::string>(data_);
        break;
    case x_stdtm:
        sz = get_vector_size<std::tm>(data_);
        break;
    case x_xmltype:
        sz = get_vector_size<xml_type>(data_);
        break;
    case x_longstring:
        sz = get_vector_size<long_string>(data_);
        break;
    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void postgresql_vector_into_type_backend::clean_up()
{
    // nothing to do here
}
