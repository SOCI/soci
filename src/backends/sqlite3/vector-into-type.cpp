//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifdef _MSC_VER
#pragma warning(disable : 4512)
#endif

#define SOCI_SQLITE3_SOURCE
#include "soci-cstrtoi.h"
#include "soci-dtocstr.h"
#include "soci-exchange-cast.h"
#include "soci/blob.h"
#include "soci/rowid.h"
#include "soci/soci-platform.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "soci-cstrtod.h"
#include "soci-mktime.h"
#include "common.h"
// std
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <vector>

namespace soci
{

void sqlite3_vector_into_type_backend::define_by_pos(
    int& position, void* data, details::exchange_type type)
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

// MSVS 2015 (only) gives a bogus warning about unreachable code here, suppress
// it to allow compilation with /WX in the CI builds.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702) // unreachable code
#endif

template <typename T>
void set_in_vector(void* p, int indx, T const& val)
{
    std::vector<T> &v = *static_cast<std::vector<T>*>(p);
    v[indx] = val;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

template <typename T>
T parse_number_from_string(const char* str)
{
    T value;
    if (!details::cstring_to_integer(value, str))
        throw soci_error("Cannot convert data");

    return value;
}

template <>
double parse_number_from_string(const char* str)
{
    return details::cstring_to_double(str);
}

template <typename T>
void set_number_in_vector(void *p, int idx, const sqlite3_column &col)
{
    using namespace details;
    using namespace details::sqlite3;

    switch (col.dataType_)
    {
        case db_date:
        case db_string:
        case db_blob:
            set_in_vector(p, idx,
                          parse_number_from_string<T>(col.buffer_.size_ > 0
                                                        ? col.buffer_.constData_
                                                        : ""));
            break;

        case db_double:
            set_in_vector(p, idx, static_cast<T>(col.double_));
            break;

        case db_int8:
            set_in_vector(p, idx, static_cast<T>(col.int8_));
            break;
        case db_uint8:
            set_in_vector(p, idx, static_cast<T>(col.uint8_));
            break;
        case db_int16:
            set_in_vector(p, idx, static_cast<T>(col.int16_));
            break;
        case db_uint16:
            set_in_vector(p, idx, static_cast<T>(col.uint16_));
            break;
        case db_int32:
            set_in_vector(p, idx, static_cast<T>(col.int32_));
            break;
        case db_uint32:
            set_in_vector(p, idx, static_cast<T>(col.uint32_));
            break;
        case db_int64:
            set_in_vector(p, idx, static_cast<T>(col.int64_));
            break;
        case db_uint64:
            set_in_vector(p, idx, static_cast<T>(col.uint64_));
            break;

        case db_xml:
            throw soci_error("XML data type is not supported");
    };
}

} // namespace anonymous

void sqlite3_vector_into_type_backend::post_fetch(bool gotData, indicator * ind)
{
    using namespace details;
    using namespace details::sqlite3;

    if (!gotData)
    {
        // no data retrieved
        return;
    }

    int const endRow = static_cast<int>(statement_.dataCache_.size());
    for (int i = 0; i < endRow; ++i)
    {
        sqlite3_column &col = statement_.dataCache_[i][position_-1];

        if (col.isNull_)
        {
            if (ind == NULL)
            {
                throw soci_error(
                    "Null value fetched and no indicator defined.");
            }
            ind[i] = i_null;

            // nothing to do for null value, go to next row
            continue;
        }

        if (ind != NULL)
            ind[i] = i_ok;

        // conversion
        switch (type_)
        {
            case x_char:
            {
                switch (col.dataType_)
                {
                    case db_date:
                    case db_string:
                    case db_blob:
                        set_in_vector(data_, i, (col.buffer_.size_ > 0 ? col.buffer_.constData_[0] : '\0'));
                        break;

                    case db_double:
                        set_in_vector(data_, i, double_to_cstring(col.double_)[0]);
                        break;

                    case db_int8:
                    {
                        std::ostringstream ss;
                        ss << col.int8_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_uint8:
                    {
                        std::ostringstream ss;
                        ss << col.uint8_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_int16:
                    {
                        std::ostringstream ss;
                        ss << col.int16_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_uint16:
                    {
                        std::ostringstream ss;
                        ss << col.uint16_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_int32:
                    {
                        std::ostringstream ss;
                        ss << col.int32_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_uint32:
                    {
                        std::ostringstream ss;
                        ss << col.uint32_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_int64:
                    {
                        std::ostringstream ss;
                        ss << col.int64_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }
                    case db_uint64:
                    {
                        std::ostringstream ss;
                        ss << col.uint64_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }

                    case db_xml:
                        throw soci_error("XML data type is not supported");
                };
                break;
            } // x_char

            case x_stdstring:
            {
                switch (col.dataType_)
                {
                    case db_date:
                    case db_string:
                    case db_blob:
                        set_in_vector(data_, i, std::string(col.buffer_.constData_, col.buffer_.size_));
                        break;

                    case db_double:
                        set_in_vector(data_, i, double_to_cstring(col.double_));
                        break;

                    case db_int8:
                    {
                        std::ostringstream ss;
                        ss << col.int8_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_uint8:
                    {
                        std::ostringstream ss;
                        ss << col.uint8_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_int16:
                    {
                        std::ostringstream ss;
                        ss << col.int16_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_uint16:
                    {
                        std::ostringstream ss;
                        ss << col.uint16_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_int32:
                    {
                        std::ostringstream ss;
                        ss << col.int32_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_uint32:
                    {
                        std::ostringstream ss;
                        ss << col.uint32_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_int64:
                    {
                        std::ostringstream ss;
                        ss << col.int64_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }
                    case db_uint64:
                    {
                        std::ostringstream ss;
                        ss << col.uint64_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }

                    case db_xml:
                    {
                        soci::xml_type xml;
                        xml.value = std::string(col.buffer_.constData_, col.buffer_.size_);
                        set_in_vector(data_, i, xml);
                        break;
                    }
                };
                break;
            } // x_stdstring

            case x_xmltype:
            {
                switch (col.dataType_)
                {
                    case db_string:
                    case db_blob:
                    case db_xml:
                    {
                        soci::xml_type xml;
                        xml.value = std::string(col.buffer_.constData_, col.buffer_.size_);
                        set_in_vector(data_, i, xml);
                        break;
                    }
                    default:
                        throw soci_error("DB type does not have a valid conversion to expected XML type");
                };
                break;
            } // x_xmltype

            case x_int8:
                set_number_in_vector<exchange_type_traits<x_int8>::value_type>(data_, i, col);
                break;

            case x_uint8:
                set_number_in_vector<exchange_type_traits<x_uint8>::value_type>(data_, i, col);
                break;

            case x_int16:
                set_number_in_vector<exchange_type_traits<x_int16>::value_type>(data_, i, col);
                break;

            case x_uint16:
                set_number_in_vector<exchange_type_traits<x_uint16>::value_type>(data_, i, col);
                break;

            case x_int32:
                set_number_in_vector<exchange_type_traits<x_int32>::value_type>(data_, i, col);
                break;

            case x_uint32:
                set_number_in_vector<exchange_type_traits<x_uint32>::value_type>(data_, i, col);
                break;

            case x_int64:
                set_number_in_vector<exchange_type_traits<x_int64>::value_type>(data_, i, col);
                break;

            case x_uint64:
                set_number_in_vector<exchange_type_traits<x_uint64>::value_type>(data_, i, col);
                break;

            case x_double:
                set_number_in_vector<exchange_type_traits<x_double>::value_type>(data_, i, col);
                break;

            case x_stdtm:
            {
                switch (col.dataType_)
                {
                    case db_date:
                    case db_string:
                    case db_blob:
                    {
                        // attempt to parse the string and convert to std::tm
                        std::tm t = std::tm();
                        parse_std_tm(col.buffer_.constData_, t);

                        set_in_vector(data_, i, t);
                        break;
                    }

                    case db_double:
                    case db_int8:
                    case db_uint8:
                    case db_int16:
                    case db_uint16:
                    case db_int32:
                    case db_uint32:
                    case db_int64:
                    case db_uint64:
                        throw soci_error("Into element used with non-convertible type.");

                    case db_xml:
                        throw soci_error("XML data type is not supported");
                };
                break;
            }

            default:
                throw soci_error("Into element used with non-supported type.");
        }

        // cleanup data
        switch (col.dataType_)
        {
            case db_date:
            case db_string:
            case db_blob:
                delete[] col.buffer_.data_;
                col.buffer_.data_ = NULL;
                break;

            case db_double:
            case db_int8:
            case db_uint8:
            case db_int16:
            case db_uint16:
            case db_int32:
            case db_uint32:
            case db_int64:
            case db_uint64:
                break;

            case db_xml:
                throw soci_error("XML data type is not supported");
        }
    }
}

void sqlite3_vector_into_type_backend::resize(std::size_t sz)
{
    using namespace details;
    using namespace details::sqlite3;

    switch (type_)
    {
        // simple cases
    case x_char:
        resize_vector<char>(data_, sz);
        break;
    case x_int8:
        resize_vector<int8_t>(data_, sz);
        break;
    case x_uint8:
        resize_vector<uint8_t>(data_, sz);
        break;
    case x_int16:
        resize_vector<int16_t>(data_, sz);
        break;
    case x_uint16:
        resize_vector<uint16_t>(data_, sz);
        break;
    case x_int32:
        resize_vector<int32_t>(data_, sz);
        break;
    case x_uint32:
        resize_vector<uint32_t>(data_, sz);
        break;
    case x_int64:
        resize_vector<int64_t>(data_, sz);
        break;
    case x_uint64:
        resize_vector<uint64_t>(data_, sz);
        break;
    case x_double:
        resize_vector<double>(data_, sz);
        break;
    case x_stdstring:
        resize_vector<std::string>(data_, sz);
        break;
    case x_stdtm:
        resize_vector<std::tm>(data_, sz);
        break;
    case x_xmltype:
        resize_vector<soci::xml_type>(data_, sz);
        break;
    default:
        throw soci_error("Into vector element used with non-supported type.");
    }
}

std::size_t sqlite3_vector_into_type_backend::size()
{
    using namespace details;
    using namespace details::sqlite3;

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
        sz = get_vector_size<uint64_t>(data_);
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
        sz = get_vector_size<soci::xml_type>(data_);
        break;
    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void sqlite3_vector_into_type_backend::clean_up()
{
}

} // namespace soci
