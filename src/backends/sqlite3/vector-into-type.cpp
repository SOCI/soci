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
#include "soci/soci-platform.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "soci-cstrtod.h"
#include "soci-mktime.h"
#include "common.h"
// std
#include <algorithm>
#include <cstddef>
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

    switch (col.type_)
    {
        case dt_date:
        case dt_string:
        case dt_blob:
            set_in_vector(p, idx,
                          parse_number_from_string<T>(col.buffer_.size_ > 0
                                                        ? col.buffer_.constData_
                                                        : ""));
            break;

        case dt_double:
            set_in_vector(p, idx, static_cast<T>(col.double_));
            break;

        case dt_integer:
            set_in_vector(p, idx, static_cast<T>(col.int32_));
            break;

        case dt_long_long:
        case dt_unsigned_long_long:
            set_in_vector(p, idx, static_cast<T>(col.int64_));
            break;

        case dt_xml:
            throw soci_error("XML data type is not supported");
    };
}

} // namespace anonymous

sqlite3_column sqlite3_vector_into_type_backend::get_column(int colNum)
{
    sqlite3_column col;
    {
        data_type type;
        std::string columnName;
        statement_.describe_column(position_, type, columnName);
        col.isNull_ = (sqlite3_column_type(statement_.stmt_, colNum) == SQLITE_NULL);
        col.type_ = type;
    }
    if (col.isNull_)
        return col;
    switch (col.type_)
    {
        case dt_date:
        case dt_string:
        case dt_blob:
        case dt_xml:
            col.buffer_.size_ = sqlite3_column_bytes(statement_.stmt_, colNum);
            col.buffer_.data_ = new char[col.buffer_.size_ + 1];
            memcpy(col.buffer_.data_, sqlite3_column_text(statement_.stmt_, colNum), col.buffer_.size_ + 1);
            break;

        case dt_double:
            col.double_ = details::cstring_to_double(reinterpret_cast<const char*>(sqlite3_column_text(statement_.stmt_, colNum)));
            break;

        case dt_integer:
        case dt_long_long:
        case dt_unsigned_long_long:
        {
            details::cstring_to_integer(col.int64_, reinterpret_cast<const char*>(sqlite3_column_text(statement_.stmt_, colNum)));
            break;
            }
        }
    return col;
}

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
        std::shared_ptr<sqlite3_column> col1;
        if (1 == endRow)
        {
            col1 = std::make_unique<sqlite3_column>(get_column(position_ - 1));
        }
        else
        {
            col1 = std::make_unique<sqlite3_column>(statement_.dataCache_[i][position_ - 1]);
        }
        sqlite3_column& col = *col1;
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
                switch (col.type_)
                {
                    case dt_date:
                    case dt_string:
                    case dt_blob:
                        set_in_vector(data_, i, (col.buffer_.size_ > 0 ? col.buffer_.constData_[0] : '\0'));
                        break;

                    case dt_double:
                        set_in_vector(data_, i, double_to_cstring(col.double_)[0]);
                        break;

                    case dt_integer:
                    {
                        std::ostringstream ss;
                        ss << col.int32_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }

                    case dt_long_long:
                    case dt_unsigned_long_long:
                    {
                        std::ostringstream ss;
                        ss << col.int64_;
                        set_in_vector(data_, i, ss.str()[0]);
                        break;
                    }

                    case dt_xml:
                        throw soci_error("XML data type is not supported");
                };
                break;
            } // x_char

            case x_stdstring:
            {
                switch (col.type_)
                {
                    case dt_date:
                    case dt_string:
                    case dt_blob:
                        set_in_vector(data_, i, std::string(col.buffer_.constData_, col.buffer_.size_));
                        break;

                    case dt_double:
                        set_in_vector(data_, i, double_to_cstring(col.double_));
                        break;

                    case dt_integer:
                    {
                        std::ostringstream ss;
                        ss << col.int32_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }

                    case dt_long_long:
                    case dt_unsigned_long_long:
                    {
                        std::ostringstream ss;
                        ss << col.int64_;
                        set_in_vector(data_, i, ss.str());
                        break;
                    }

                    case dt_xml:
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
                switch (col.type_)
                {
                    case dt_string:
                    case dt_blob:
                    case dt_xml:
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

            case x_short:
                set_number_in_vector<exchange_type_traits<x_short>::value_type>(data_, i, col);
                break;

            case x_integer:
                set_number_in_vector<exchange_type_traits<x_integer>::value_type>(data_, i, col);
                break;

            case x_long_long:
                set_number_in_vector<exchange_type_traits<x_long_long>::value_type>(data_, i, col);
                break;

            case x_unsigned_long_long:
                set_number_in_vector<exchange_type_traits<x_unsigned_long_long>::value_type>(data_, i, col);
                break;

            case x_double:
                set_number_in_vector<exchange_type_traits<x_double>::value_type>(data_, i, col);
                break;

            case x_stdtm:
            {
                switch (col.type_)
                {
                    case dt_date:
                    case dt_string:
                    case dt_blob:
                    {
                        // attempt to parse the string and convert to std::tm
                        std::tm t = std::tm();
                        parse_std_tm(col.buffer_.constData_, t);

                        set_in_vector(data_, i, t);
                        break;
                    }

                    case dt_double:
                    case dt_integer:
                    case dt_long_long:
                    case dt_unsigned_long_long:
                        throw soci_error("Into element used with non-convertible type.");

                    case dt_xml:
                        throw soci_error("XML data type is not supported");
                };
                break;
            }

            default:
                throw soci_error("Into element used with non-supported type.");
        }

        // cleanup data
        switch (col.type_)
        {
            case dt_date:
            case dt_string:
            case dt_blob:
                delete[] col.buffer_.data_;
                col.buffer_.data_ = NULL;
                break;

            case dt_double:
            case dt_integer:
            case dt_long_long:
            case dt_unsigned_long_long:
                break;

            case dt_xml:
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
    case x_short:
        resize_vector<short>(data_, sz);
        break;
    case x_integer:
        resize_vector<int>(data_, sz);
        break;
    case x_long_long:
        resize_vector<long long>(data_, sz);
        break;
    case x_unsigned_long_long:
        resize_vector<unsigned long long>(data_, sz);
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
    case x_short:
        sz = get_vector_size<short>(data_);
        break;
    case x_integer:
        sz = get_vector_size<int>(data_);
        break;
    case x_long_long:
        sz = get_vector_size<long long>(data_);
        break;
    case x_unsigned_long_long:
        sz = get_vector_size<unsigned long long>(data_);
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
