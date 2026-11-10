//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci-platform.h"
#include "soci/odbc/soci-odbc.h"
#include "soci/soci-unicode.h"
#include "soci/type-wrappers.h"
#include "soci-compiler.h"
#include "soci-cstrtoi.h"
#include "soci-mktime.h"
#include "soci-vector-helpers.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <fmt/format.h>

using namespace soci;
using namespace soci::details;

void odbc_vector_into_type_backend::define_by_pos(
    int &position, void *data, exchange_type type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference
    position_ = position - 1;

    statement_.intos_.push_back(this);

    const std::size_t vectorSize = get_vector_size(type, data);
    if (vectorSize == 0)
    {
         throw soci_error("Vectors of size 0 are not allowed.");
    }

    indHolderVec_.resize(vectorSize);

    switch (type)
    {
    // simple cases
    case x_int8:
        odbcType_ = SQL_C_STINYINT;
        break;
    case x_uint8:
        odbcType_ = SQL_C_UTINYINT;
        break;
    case x_int16:
        odbcType_ = SQL_C_SSHORT;
        break;
    case x_uint16:
        odbcType_ = SQL_C_USHORT;
        break;
    case x_int32:
        odbcType_ = SQL_C_SLONG;
        static_assert(sizeof(SQLINTEGER) == sizeof(int32_t), "unsupported SQLINTEGER size");
        break;
    case x_uint32:
        odbcType_ = SQL_C_ULONG;
        break;
    case x_int64:
        if (use_string_for_bigint())
        {
            odbcType_ = SQL_C_CHAR;
            colSize_ = max_bigint_length;
            buf_ = new char[colSize_ * vectorSize];
        }
        else // Normal case, use ODBC support.
        {
            odbcType_ = SQL_C_SBIGINT;
        }
        break;
    case x_uint64:
        if (use_string_for_bigint())
        {
            odbcType_ = SQL_C_CHAR;
            colSize_ = max_bigint_length;
            buf_ = new char[colSize_ * vectorSize];
        }
        else // Normal case, use ODBC support.
        {
            odbcType_ = SQL_C_UBIGINT;
        }
        break;
    case x_double:
        odbcType_ = SQL_C_DOUBLE;
        break;

    // cases that require adjustments and buffer management

    case x_char:
        odbcType_ = SQL_C_CHAR;

        colSize_ = sizeof(char) * 2;
        buf_ = new char[colSize_ * vectorSize];
        break;
    case x_stdstring:
    case x_xmltype:
    case x_longstring:
        odbcType_ = SQL_C_CHAR;
        SOCI_FALLTHROUGH;
    case x_stdwstring:
        {
            if (type_ == x_stdwstring)
                odbcType_ = SQL_C_WCHAR;

            colSize_ = static_cast<size_t>(get_sqllen_from_value(statement_.column_size(position)));
            if (is_odbc_long_column(colSize_))
            {
                // Column size for text data type can be too large for buffer
                // allocation (or unknown), so we don't allocate any buffer
                // and retrieve the values using SQLGetData() instead.
                isLong_ = true;
                colSize_ = 0;
                break;
            }

            colSize_++;
            if (type_ == x_stdwstring)
                colSize_ *= sizeof(SQLWCHAR); // size in bytes

            // If we are fetching by a single row, allocate the buffer only for
            // one value.
            const std::size_t elementsCount
                = statement_.fetchVectorByRows_ ? 1 : vectorSize;
            buf_ = new char[colSize_ * elementsCount];
        }
        break;
    case x_stdtm:
        odbcType_ = SQL_C_TYPE_TIMESTAMP;

        colSize_ = sizeof(TIMESTAMP_STRUCT);
        buf_ = new char[colSize_ * vectorSize];
        break;

    default:
        throw soci_error("Into element used with non-supported type.");
    }

    if (isLong_)
        statement_.add_long_column(position_ + 1);

    if (uses_get_data())
    {
        // SQLGetData() can only be used for retrieving the values of the
        // current row, so we need to fetch the rows one by one, see the
        // comment in odbc_statement_backend::fetch().
        //
        // Note that the flag is permanent for the statement and will never
        // be reset.
        statement_.fetchVectorByRows_ = true;
    }

    position++;

    rebind_row(0);
}

void* odbc_vector_into_type_backend::get_row_buffer(std::size_t rowInd, SQLLEN& size)
{
    void* elementPtr = nullptr;
    size = 0;
    switch (type_)
    {
    // simple cases
    case x_int8:
        elementPtr = &exchange_vector_type_cast<x_int8>(data_)[rowInd];
        size = sizeof(int8_t);
        break;
    case x_uint8:
        elementPtr = &exchange_vector_type_cast<x_uint8>(data_)[rowInd];
        size = sizeof(uint8_t);
        break;
    case x_int16:
        elementPtr = &exchange_vector_type_cast<x_int16>(data_)[rowInd];
        size = sizeof(int16_t);
        break;
    case x_uint16:
        elementPtr = &exchange_vector_type_cast<x_uint16>(data_)[rowInd];
        size = sizeof(uint16_t);
        break;
    case x_int32:
        elementPtr = &exchange_vector_type_cast<x_int32>(data_)[rowInd];
        size = sizeof(SQLINTEGER);
        break;
    case x_uint32:
        elementPtr = &exchange_vector_type_cast<x_uint32>(data_)[rowInd];
        size = sizeof(SQLINTEGER);
        break;
    case x_int64:
        if (!use_string_for_bigint())
        {
            elementPtr = &exchange_vector_type_cast<x_int64>(data_)[rowInd];
            size = sizeof(int64_t);
        }
        break;
    case x_uint64:
        if (!use_string_for_bigint())
        {
            elementPtr = &exchange_vector_type_cast<x_uint64>(data_)[rowInd];
            size = sizeof(uint64_t);
        }
        break;
    case x_double:
        elementPtr = &exchange_vector_type_cast<x_double>(data_)[rowInd];
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management

    case x_char:
    case x_stdstring:
    case x_stdwstring:
    case x_xmltype:
    case x_longstring:
    case x_stdtm:
        // Do nothing.
        break;

    default:
        throw soci_error("Into element used with non-supported type.");
    }

    if (elementPtr == nullptr)
    {
        // It's one of the types for which we use fixed buffer.
        elementPtr = buf_;
        size = static_cast<SQLLEN>(colSize_);
    }

    return elementPtr;
}

bool odbc_vector_into_type_backend::uses_get_data() const
{
    // Note that position_ is 0-based, unlike the column indices.
    return statement_.is_get_data_column(position_ + 1);
}

void odbc_vector_into_type_backend::rebind_row(std::size_t rowInd)
{
    // Columns retrieved using SQLGetData() are never bound.
    if (uses_get_data())
        return;

    SQLLEN size;
    void* const elementPtr = get_row_buffer(rowInd, size);

    const SQLUSMALLINT pos = static_cast<SQLUSMALLINT>(position_ + 1);
    SQLRETURN rc
        = SQLBindCol(statement_.hstmt_, pos, odbcType_,
            static_cast<SQLPOINTER>(elementPtr), size, &indHolderVec_[rowInd]);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, fmt::format("binding output vector item at index {} of column #{}", rowInd, pos));
    }
}

void odbc_vector_into_type_backend::get_data(std::size_t rowInd)
{
    const SQLUSMALLINT pos = static_cast<SQLUSMALLINT>(position_ + 1);

    if (isLong_)
    {
        // Retrieve the value directly into the vector element.
        bool gotValue;
        std::size_t len = 0;
        //
        // For consistency with the values of the bound columns, see
        // do_post_fetch_rows(), trim the trailing spaces and clear the
        // elements corresponding to NULL values.
        if (type_ == x_stdwstring)
        {
            std::wstring& value
                = exchange_vector_type_cast<x_stdwstring>(data_).at(rowInd);

            std::u16string s;
            gotValue = get_long_data(pos, s);
            if (gotValue)
            {
                s.erase(s.find_last_not_of(u' ') + 1);
                len = s.size();

                value = utf16_to_wide(s);
            }
            else
            {
                value.clear();
            }
        }
        else
        {
            std::string& value = vector_string_value(type_, data_, rowInd);
            gotValue = get_long_data(pos, value);
            if (gotValue)
            {
                value.erase(value.find_last_not_of(' ') + 1);
                len = value.size();
            }
            else
            {
                value.clear();
            }
        }

        // Set the indicator as it would have been set by SQLBindCol(),
        // post_fetch() relies on it being SQL_NULL_DATA for NULL values.
        set_sqllen_at(rowInd, gotValue ? static_cast<SQLLEN>(len) : SQL_NULL_DATA);
        return;
    }

    SQLLEN size;
    void* const elementPtr = get_row_buffer(rowInd, size);

    SQLLEN ind = 0;
    SQLRETURN rc = SQLGetData(statement_.hstmt_, pos, odbcType_,
                              elementPtr, size, &ind);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                              fmt::format("getting data of vector item at index {} of column #{}", rowInd, pos));
    }

    set_sqllen_at(rowInd, get_sqllen_from_value(ind));
}

void odbc_vector_into_type_backend::pre_fetch()
{
    // nothing to do for the supported types
}

void odbc_vector_into_type_backend::do_post_fetch_rows(
    std::size_t beginRow, std::size_t endRow)
{
    if (uses_get_data())
    {
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            get_data(i);
        }

        // Values of long columns are retrieved directly into the vector
        // elements, so there is nothing more to do for them.
        if (isLong_)
            return;
    }

    if (type_ == x_char)
    {
        std::vector<char> *vp
            = static_cast<std::vector<char> *>(data_);

        std::vector<char> &v(*vp);
        char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            v[i] = *pos;
            pos += colSize_;
        }
    }
    if (type_ == x_stdstring || type_ == x_xmltype || type_ == x_longstring)
    {
        const char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i, pos += colSize_)
        {
            SQLLEN const len = get_sqllen_from_vector_at(i);

            std::string& value = vector_string_value(type_, data_, i);
            if (len == -1)
            {
                // Value is null.
                value.clear();
                continue;
            }

            // Find the actual length of the string: for a VARCHAR(N)
            // column, it may be right-padded with spaces up to the length
            // of the longest string in the result set. This happens with
            // at least MS SQL (and the exact behaviour depends on the
            // value of the ANSI_PADDING option) and it seems like some
            // other ODBC drivers also have options like "PADVARCHAR", so
            // it's probably not the only case when it does.
            //
            // So deal with this generically by just trimming all the
            // spaces from the right hand-side.
            const char* end = pos + len;
            while (end != pos)
            {
                // Pre-decrement as "end" is one past the end, as usual.
                if (*--end != ' ')
                {
                    // We must count the last non-space character.
                    ++end;
                    break;
                }
            }

            value.assign(pos, end - pos);
        }
    }
    else if (type_ == x_stdwstring)
    {
        // Do exactly the same thing as above, but for wide characters.
        SQLWCHAR* pos = reinterpret_cast<SQLWCHAR*>(buf_);
        for (std::size_t i = beginRow; i != endRow; ++i, pos += colSize_ / sizeof(SQLWCHAR))
        {
            SQLLEN len = get_sqllen_from_vector_at(i);

            std::wstring& value = exchange_vector_type_cast<x_stdwstring>(data_).at(i);
            if (len == -1)
            {
                value.clear();
                continue;
            }

            SQLWCHAR* end = pos + len / sizeof(SQLWCHAR);
            while (end != pos)
            {
                if (*--end != L' ')
                {
                    ++end;
                    break;
                }
            }

            value = utf16_to_wide(reinterpret_cast<char16_t*>(pos), end - pos);
        }
    }
    else if (type_ == x_stdtm)
    {
        std::vector<std::tm> *vp
            = static_cast<std::vector<std::tm> *>(data_);

        std::vector<std::tm> &v(*vp);
        char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            // See comment for the use of this macro in standard-into-type.cpp.
            SOCI_GCC_WARNING_SUPPRESS(cast-align)

            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(pos);

            SOCI_GCC_WARNING_RESTORE(cast-align)

            details::mktime_from_ymdhms(v[i],
                                        ts->year, ts->month, ts->day,
                                        ts->hour, ts->minute, ts->second);
            pos += colSize_;
        }
    }
    else if (type_ == x_int64 && use_string_for_bigint())
    {
        std::vector<int64_t> *vp
            = static_cast<std::vector<int64_t> *>(data_);
        std::vector<int64_t> &v(*vp);
        char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            if (!cstring_to_integer(v[i], pos))
            {
                throw soci_error("Failed to parse the returned 64-bit integer value");
            }
            pos += colSize_;
        }
    }
    else if (type_ == x_uint64 && use_string_for_bigint())
    {
        std::vector<uint64_t> *vp
            = static_cast<std::vector<uint64_t> *>(data_);
        std::vector<uint64_t> &v(*vp);
        char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            if (!cstring_to_unsigned(v[i], pos))
            {
                throw soci_error("Failed to parse the returned 64-bit integer value");
            }
            pos += colSize_;
        }
    }
}

void odbc_vector_into_type_backend::post_fetch(bool gotData, indicator* ind)
{
    // Here we have to set indicators only. Data was exchanged with user
    // buffers during fetch()
    if (gotData)
    {
        std::size_t rows = statement_.numRowsFetched_;

        for (std::size_t i = 0; i < rows; ++i)
        {
            SQLLEN const val = get_sqllen_from_vector_at(i);
            if (val == SQL_NULL_DATA)
            {
                if (ind == nullptr)
                {
                    throw soci_error("Null value fetched and no indicator defined.");
                }

                ind[i] = i_null;
            }
            else if (ind != nullptr)
            {
                ind[i] = i_ok;
            }
        }
    }
}

void odbc_vector_into_type_backend::resize(std::size_t sz)
{
    // stays 64bit but gets but casted, see: get_sqllen_from_vector_at(...)
    indHolderVec_.resize(sz);
    resize_vector(type_, data_, sz);
}

std::size_t odbc_vector_into_type_backend::size() const
{
    return get_vector_size(type_, data_);
}

void odbc_vector_into_type_backend::clean_up()
{
    if (buf_ != nullptr)
    {
        delete [] buf_;
        buf_ = nullptr;
    }
    std::vector<odbc_vector_into_type_backend*>::iterator it
        = std::find(statement_.intos_.begin(), statement_.intos_.end(), this);
    if (it != statement_.intos_.end())
        statement_.intos_.erase(it);
}
