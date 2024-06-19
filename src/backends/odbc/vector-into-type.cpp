//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
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
#include <sstream>

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
    // Handle the case where the data type is wide character (wchar)
    case x_wchar:
        // Set the ODBC type to SQL_C_WCHAR, which represents a wide character string
        odbcType_ = SQL_C_WCHAR;

        // Calculate the column size for wide characters. 
        // SQLWCHAR is typically 2 bytes, so we multiply by 2 to get the size in bytes.
        colSize_ = sizeof(SQLWCHAR) * 2;

        // Allocate memory for the buffer to hold the wide character data.
        // The buffer size is calculated as colSize_ multiplied by the number of elements (vectorSize).
        buf_ = new char[colSize_ * vectorSize];
        break;
    case x_stdstring:
    case x_xmltype:
    case x_longstring:
    {
        odbcType_ = SQL_C_CHAR;

        colSize_ = static_cast<size_t>(get_sqllen_from_value(statement_.column_size(position)));
        if (colSize_ >= ODBC_MAX_COL_SIZE || colSize_ == 0)
        {
            // Column size for text data type can be too large for buffer allocation.
            colSize_ = odbc_max_buffer_length;
            // If we are using huge buffer size then we need to fetch rows
            // one by one as otherwise we could easily run out of memory.
            // Note that the flag is permanent for the statement and will
            // never be reset.
            statement_.fetchVectorByRows_ = true;
        }

        colSize_++;

        // If we are fetching by a single row, allocate the buffer only for
        // one value.
        const std::size_t elementsCount
            = statement_.fetchVectorByRows_ ? 1 : vectorSize;
        buf_ = new char[colSize_ * elementsCount];
    }
    break;
    case x_stdwstring:
    {
        // Set the ODBC type to wide character string (SQL_C_WCHAR).
        odbcType_ = SQL_C_WCHAR;

        // Retrieve the column size from the statement for the given position.
        colSize_ = static_cast<size_t>(get_sqllen_from_value(statement_.column_size(position)));
        
        // Check if the column size is too large or zero.
        if (colSize_ >= ODBC_MAX_COL_SIZE || colSize_ == 0)
        {
            // If the column size is too large or zero, set it to a maximum buffer length.
            colSize_ = odbc_max_buffer_length;
            
            // If using a huge buffer size, fetch rows one by one to avoid running out of memory.
            // This flag is permanent for the statement and will not be reset.
            statement_.fetchVectorByRows_ = true;
        }

        // Add space for the null terminator for wide characters.
        colSize_ += sizeof(SQLWCHAR);

        // Determine the number of elements to allocate space for.
        // If fetching by a single row, allocate buffer only for one value.
        const std::size_t elementsCount = statement_.fetchVectorByRows_ ? 1 : vectorSize;
        
        // Allocate memory for the buffer to hold the wide character strings.
        // The buffer size is calculated as column size times the number of elements,
        // each element being of size SQLWCHAR.
        buf_ = new char[colSize_ * elementsCount * sizeof(SQLWCHAR)];
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

    position++;

    rebind_row(0);
}

void odbc_vector_into_type_backend::rebind_row(std::size_t rowInd)
{
    void* elementPtr = NULL;
    SQLLEN size = 0;
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
    case x_wchar:
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

    if (elementPtr == NULL)
    {
        // It's one of the types for which we use fixed buffer.
        elementPtr = buf_;
        size = colSize_;
    }

    const SQLUSMALLINT pos = static_cast<SQLUSMALLINT>(position_ + 1);
    SQLRETURN rc
        = SQLBindCol(statement_.hstmt_, pos, odbcType_,
            static_cast<SQLPOINTER>(elementPtr), size, &indHolderVec_[rowInd]);
    if (is_odbc_error(rc))
    {
        std::ostringstream ss;
        ss << "binding output vector item at index " << rowInd
           << " of column #" << pos;
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, ss.str());
    }
}

void odbc_vector_into_type_backend::pre_fetch()
{
    // nothing to do for the supported types
}

void odbc_vector_into_type_backend::do_post_fetch_rows(
    std::size_t beginRow, std::size_t endRow)
{
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
    // Check if the type is wide character (wchar_t)
    if (type_ == x_wchar)
    {
        // Cast the data_ pointer to a vector of wchar_t
        std::vector<wchar_t> *vp = static_cast<std::vector<wchar_t> *>(data_);
        // Create a reference to the vector for easier access
        std::vector<wchar_t> &v(*vp);
        
        // Initialize a pointer to the buffer
        char *pos = buf_;
        // Loop through the specified range of rows
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            // Check if the platform defines wchar_t as wide (e.g., Unix systems)
#if defined(SOCI_WCHAR_T_IS_WIDE) // Unices
            // Convert UTF-16 to UTF-32 and assign the first character to the vector
            v[i] = utf16_to_utf32(std::u16string(reinterpret_cast<char16_t*>(pos)))[0];
#else
            // Directly reinterpret the buffer as wchar_t and assign to the vector
            v[i] = *reinterpret_cast<wchar_t*>(pos);
#endif // SOCI_WCHAR_T_IS_WIDE
            // Move the buffer pointer to the next column size
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
        // Cast the buffer to SQLWCHAR* for wide character processing.
        SQLWCHAR* pos = reinterpret_cast<SQLWCHAR*>(buf_);
        // Calculate the column size in terms of SQLWCHAR.
        std::size_t const colSize = colSize_ / sizeof(SQLWCHAR);

        // Iterate over the rows from beginRow to endRow.
        for (std::size_t i = beginRow; i != endRow; ++i, pos += colSize)
        {
            // Get the length of the current element in the vector.
            SQLLEN len = get_sqllen_from_vector_at(i);

            // Reference to the current std::wstring element in the vector.
            std::wstring& value = exchange_vector_type_cast<x_stdwstring>(data_).at(i);
            
            if (len == -1)
            {
                // If length is -1, the value is null. Clear the string.
                value.clear();
                continue;
            }
            else
            {
                // Adjust length to account for wide characters.
                len = len / sizeof(SQLWCHAR);
            }

            // Calculate the end position of the current string.
            SQLWCHAR* end = pos + len;
            
            // Trim trailing spaces from the string.
            while (end != pos)
            {
                // Pre-decrement as "end" is one past the end, as usual.
                if (*--end != L' ')
                {
                    // We must count the last non-space character.
                    ++end;
                    break;
                }
            }

#if defined(SOCI_WCHAR_T_IS_WIDE) // Unix-like systems
            // Convert UTF-16 to UTF-32 and assign to the std::wstring.
            const std::u32string u32str(utf16_to_utf32(std::u16string(reinterpret_cast<char16_t*>(pos), end - pos)));
            value.assign(u32str.begin(), u32str.end());
#else // Windows
            // Directly assign the wide character string to std::wstring.
            value.assign(reinterpret_cast<wchar_t const*>(pos), end - pos);
#endif // SOCI_WCHAR_T_IS_WIDE
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
                if (ind == NULL)
                {
                    throw soci_error("Null value fetched and no indicator defined.");
                }

                ind[i] = i_null;
            }
            else if (ind != NULL)
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

std::size_t odbc_vector_into_type_backend::size()
{
    return get_vector_size(type_, data_);
}

void odbc_vector_into_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
    std::vector<odbc_vector_into_type_backend*>::iterator it
        = std::find(statement_.intos_.begin(), statement_.intos_.end(), this);
    if (it != statement_.intos_.end())
        statement_.intos_.erase(it);
}
