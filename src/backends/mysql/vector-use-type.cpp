//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"
#include "common.h"
#include "soci/soci-platform.h"
#include "soci-dtocstr.h"
#include "soci-mktime.h"
// std
#include <ciso646>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <vector>

using namespace soci;
using namespace soci::details;
using namespace soci::details::mysql;


void mysql_vector_use_type_backend::bind_by_pos(int &position, void *data,
    exchange_type type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void mysql_vector_use_type_backend::bind_by_name(
    std::string const &name, void *data, exchange_type type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void mysql_vector_use_type_backend::pre_use(indicator const *ind)
{
    std::size_t const vsize = size();
    for (size_t i = 0; i != vsize; ++i)
    {
        char *buf;

        // the data in vector can be either i_ok or i_null
        if (ind != NULL && ind[i] == i_null)
        {
            buf = new char[5];
            std::strcpy(buf, "NULL");
        }
        else
        {
            // allocate and fill the buffer with text-formatted client data
            switch (type_)
            {
            case x_char:
                {
                    std::vector<char> *pv
                        = static_cast<std::vector<char> *>(data_);
                    std::vector<char> &v = *pv;

                    char tmp[] = { v[i], '\0' };
                    buf = quote(statement_.session_.conn_, tmp, 1);
                }
                break;
            case x_stdstring:
                {
                    std::vector<std::string> *pv
                        = static_cast<std::vector<std::string> *>(data_);
                    std::vector<std::string> &v = *pv;

                    buf = quote(statement_.session_.conn_,
                        v[i].c_str(), v[i].size());
                }
                break;
            case x_int8:
                {
                    std::vector<int8_t> *pv
                        = static_cast<std::vector<int8_t> *>(data_);
                    std::vector<int8_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int8_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%d", v[i]);
                }
                break;
            case x_uint8:
                {
                    std::vector<uint8_t> *pv
                        = static_cast<std::vector<uint8_t> *>(data_);
                    std::vector<uint8_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<uint8_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%u", v[i]);
                }
                break;
            case x_int16:
                {
                    std::vector<int16_t> *pv
                        = static_cast<std::vector<int16_t> *>(data_);
                    std::vector<int16_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int16_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%d", v[i]);
                }
                break;
            case x_uint16:
                {
                    std::vector<uint16_t> *pv
                        = static_cast<std::vector<uint16_t> *>(data_);
                    std::vector<uint16_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<uint16_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%u", v[i]);
                }
                break;
            case x_int32:
                {
                    std::vector<int32_t> *pv
                        = static_cast<std::vector<int32_t> *>(data_);
                    std::vector<int32_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int32_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%d", v[i]);
                }
                break;
            case x_uint32:
                {
                    std::vector<uint32_t> *pv
                        = static_cast<std::vector<uint32_t> *>(data_);
                    std::vector<uint32_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<uint32_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%u", v[i]);
                }
                break;
            case x_int64:
                {
                    std::vector<int64_t> *pv
                        = static_cast<std::vector<int64_t> *>(data_);
                    std::vector<int64_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int64_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%" LL_FMT_FLAGS "d",
                             static_cast<long long>(v[i]));
                }
                break;
            case x_uint64:
                {
                    std::vector<uint64_t> *pv
                        = static_cast<std::vector<uint64_t> *>(data_);
                    std::vector<uint64_t> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<uint64_t>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%" LL_FMT_FLAGS "u",
                             static_cast<unsigned long long>(v[i]));
                }
                break;
            case x_double:
                {
                    std::vector<double> *pv
                        = static_cast<std::vector<double> *>(data_);
                    std::vector<double> &v = *pv;

                    if (is_infinity_or_nan(v[i])) {
                        throw soci_error(
                            "Use element used with infinity or NaN, which are "
                            "not supported by the MySQL server.");
                    }

                    std::string const s = double_to_cstring(v[i]);

                    buf = new char[s.size() + 1];
                    std::strcpy(buf, s.c_str());
                }
                break;
            case x_stdtm:
                {
                    std::vector<std::tm> *pv
                        = static_cast<std::vector<std::tm> *>(data_);
                    std::vector<std::tm> &v = *pv;

                    std::size_t const bufSize = 80;
                    buf = new char[bufSize];

                    int n = 0;
                    buf[n++] = '\'';
                    n += format_std_tm(v[i], buf + n, bufSize - n - 1);
                    buf[n++] = '\'';
                    buf[n] = '\0';
                }
                break;

            default:
                throw soci_error(
                    "Use vector element used with non-supported type.");
            }
        }

        buffers_.push_back(buf);
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buffers_[0];
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buffers_[0];
    }
}

std::size_t mysql_vector_use_type_backend::size()
{
    std::size_t sz SOCI_DUMMY_INIT(0);
    switch (type_)
    {
        // simple cases
    case x_char:         sz = get_vector_size<char>         (data_); break;
    case x_int8:         sz = get_vector_size<int8_t>       (data_); break;
    case x_uint8:        sz = get_vector_size<uint8_t>      (data_); break;
    case x_int16:        sz = get_vector_size<int16_t>      (data_); break;
    case x_uint16:       sz = get_vector_size<uint16_t>     (data_); break;
    case x_int32:        sz = get_vector_size<int32_t>      (data_); break;
    case x_uint32:       sz = get_vector_size<uint32_t>     (data_); break;
    case x_int64:        sz = get_vector_size<int64_t>      (data_); break;
    case x_uint64:       sz = get_vector_size<uint64_t>     (data_); break;
    case x_double:       sz = get_vector_size<double>       (data_); break;
    case x_stdstring:    sz = get_vector_size<std::string>  (data_); break;
    case x_stdtm:        sz = get_vector_size<std::tm>      (data_); break;

    default:
        throw soci_error("Use vector element used with non-supported type.");
    }

    return sz;
}

void mysql_vector_use_type_backend::clean_up()
{
    std::size_t const bsize = buffers_.size();
    for (std::size_t i = 0; i != bsize; ++i)
    {
        delete [] buffers_[i];
    }
}
