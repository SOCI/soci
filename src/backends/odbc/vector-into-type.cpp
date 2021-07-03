//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci/soci-platform.h"
#include "soci/odbc/soci-odbc.h"
#include "soci/type-wrappers.h"
#include "soci-compiler.h"
#include "soci-cstrtoi.h"
#include "soci-mktime.h"
#include "soci-static-assert.h"
#include "soci-vector-helpers.h"
#include <algorithm>
#include <cctype>
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
    case x_short:
        odbcType_ = SQL_C_SSHORT;
        break;
    case x_integer:
        odbcType_ = SQL_C_SLONG;
        SOCI_STATIC_ASSERT(sizeof(SQLINTEGER) == sizeof(int));
        break;
    case x_long_long:
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
    case x_unsigned_long_long:
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
    case x_short:
        elementPtr = &exchange_vector_type_cast<x_short>(data_)[rowInd];
        size = sizeof(short);
        break;
    case x_integer:
        elementPtr = &exchange_vector_type_cast<x_integer>(data_)[rowInd];
        size = sizeof(SQLINTEGER);
        break;
    case x_long_long:
        if (!use_string_for_bigint())
        {
            elementPtr = &exchange_vector_type_cast<x_long_long>(data_)[rowInd];
            size = sizeof(long long);
        }
        break;
    case x_unsigned_long_long:
        if (!use_string_for_bigint())
        {
            elementPtr = &exchange_vector_type_cast<x_unsigned_long_long>(data_)[rowInd];
            size = sizeof(unsigned long long);
        }
        break;
    case x_double:
        elementPtr = &exchange_vector_type_cast<x_double>(data_)[rowInd];
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management

    case x_char:
    case x_stdstring:
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
    else if (type_ == x_stdtm)
    {
        std::vector<std::tm> *vp
            = static_cast<std::vector<std::tm> *>(data_);

        std::vector<std::tm> &v(*vp);
        char *pos = buf_;
        for (std::size_t i = beginRow; i != endRow; ++i)
        {
            // See comment for the use of this macro in standard-into-type.cpp.
            GCC_WARNING_SUPPRESS(cast-align)

            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(pos);

            GCC_WARNING_RESTORE(cast-align)

            details::mktime_from_ymdhms(v[i],
                                        ts->year, ts->month, ts->day,
                                        ts->hour, ts->minute, ts->second);
            pos += colSize_;
        }
    }
    else if (type_ == x_long_long && use_string_for_bigint())
    {
        std::vector<long long> *vp
            = static_cast<std::vector<long long> *>(data_);
        std::vector<long long> &v(*vp);
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
    else if (type_ == x_unsigned_long_long && use_string_for_bigint())
    {
        std::vector<unsigned long long> *vp
            = static_cast<std::vector<unsigned long long> *>(data_);
        std::vector<unsigned long long> &v(*vp);
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
