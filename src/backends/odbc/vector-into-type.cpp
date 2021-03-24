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

void odbc_vector_into_type_backend::prepare_indicators(std::size_t size)
{
    if (size == 0)
    {
         throw soci_error("Vectors of size 0 are not allowed.");
    }

    indHolderVec_.resize(size);
}

void odbc_vector_into_type_backend::define_by_pos(
    int &position, void *data, exchange_type type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference
    position_ = position - 1;

    statement_.intoType_ = bt_vector;
    statement_.intos_.push_back(static_cast<void*>(this));
    statement_.inds_.push_back(std::vector<indicator>());

    SQLLEN size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case x_short:
        {
            odbcType_ = SQL_C_SSHORT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            prepare_indicators(v.size());
            data = &v[0];
        }
        break;
    case x_integer:
        {
            odbcType_ = SQL_C_SLONG;
            size = sizeof(SQLINTEGER);
            SOCI_STATIC_ASSERT(sizeof(SQLINTEGER) == sizeof(int));
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            prepare_indicators(v.size());
            data = &v[0];
        }
        break;
    case x_long_long:
        {
            std::vector<long long> *vp =
                static_cast<std::vector<long long> *>(data);
            std::vector<long long> &v(*vp);
            prepare_indicators(v.size());
            if (use_string_for_bigint())
            {
                odbcType_ = SQL_C_CHAR;
                size = max_bigint_length;
                std::size_t bufSize = size * v.size();
                colSize_ = size;
                buf_ = new char[bufSize];
                data = buf_;
            }
            else // Normal case, use ODBC support.
            {
                odbcType_ = SQL_C_SBIGINT;
                size = sizeof(long long);
                data = &v[0];
            }
        }
        break;
    case x_unsigned_long_long:
        {
            std::vector<unsigned long long> *vp =
                static_cast<std::vector<unsigned long long> *>(data);
            std::vector<unsigned long long> &v(*vp);
            prepare_indicators(v.size());
            if (use_string_for_bigint())
            {
                odbcType_ = SQL_C_CHAR;
                size = max_bigint_length;
                std::size_t bufSize = size * v.size();
                colSize_ = size;
                buf_ = new char[bufSize];
                data = buf_;
            }
            else // Normal case, use ODBC support.
            {
                odbcType_ = SQL_C_UBIGINT;
                size = sizeof(unsigned long long);
                data = &v[0];
            }
        }
        break;
    case x_double:
        {
            odbcType_ = SQL_C_DOUBLE;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepare_indicators(v.size());
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case x_char:
        {
            odbcType_ = SQL_C_CHAR;

            std::vector<char> *v
                = static_cast<std::vector<char> *>(data);

            prepare_indicators(v->size());

            size = sizeof(char) * 2;
            std::size_t bufSize = size * v->size();

            colSize_ = size;

            buf_ = new char[bufSize];
            data = buf_;
        }
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

            const std::size_t vectorSize = get_vector_size(type, data);
            // If we are fetching by a single row, allocate the buffer only for
            // one value.
            const std::size_t elementsCount
                = statement_.fetchVectorByRows_ ? 1 : vectorSize;
            std::size_t bufSize = colSize_ * elementsCount;
            buf_ = new char[bufSize];

            prepare_indicators(vectorSize);

            size = static_cast<SQLINTEGER>(colSize_);
            data = buf_;
        }
        break;
    case x_stdtm:
        {
            odbcType_ = SQL_C_TYPE_TIMESTAMP;
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data);

            prepare_indicators(v->size());

            size = sizeof(TIMESTAMP_STRUCT);
            colSize_ = size;

            std::size_t bufSize = size * v->size();

            buf_ = new char[bufSize];
            data = buf_;
        }
        break;

    default:
        throw soci_error("Into element used with non-supported type.");
    }

    SQLRETURN rc
        = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position++),
                odbcType_, static_cast<SQLPOINTER>(data), size, &indHolderVec_[0]);
    if (is_odbc_error(rc))
    {
        std::ostringstream ss;
        ss << "binding output vector column #" << position;
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, ss.str());
    }
}

void odbc_vector_into_type_backend::pre_fetch()
{
    // nothing to do for the supported types
}

void odbc_vector_into_type_backend::exchange_rows(bool gotData,
    std::size_t beginInd, std::size_t endInd)
{
    if (gotData)
    {
        // first, deal with data

        // only std::string, std::tm and Statement need special handling
        if (type_ == x_char)
        {
            std::vector<char> *vp
                = static_cast<std::vector<char> *>(data_);

            std::vector<char> &v(*vp);
            char *pos = buf_;
            for (std::size_t i = beginInd; i != endInd; ++i)
            {
                v[i] = *pos;
                pos += colSize_;
            }
        }
        if (type_ == x_stdstring || type_ == x_xmltype || type_ == x_longstring)
        {
            const char *pos = buf_;
            for (std::size_t i = beginInd; i != endInd; ++i, pos += colSize_)
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
            for (std::size_t i = beginInd; i != endInd; ++i)
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
            for (std::size_t i = beginInd; i != endInd; ++i)
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
            for (std::size_t i = beginInd; i != endInd; ++i)
            {
                if (!cstring_to_unsigned(v[i], pos))
                {
                    throw soci_error("Failed to parse the returned 64-bit integer value");
                }
                pos += colSize_;
            }
        }

        // then - deal with indicators

        for (std::size_t i = beginInd; i != endInd; ++i)
        {
            SQLLEN const val = get_sqllen_from_vector_at(i);
            if (val == SQL_NULL_DATA)
            {
                statement_.inds_[position_][i] = i_null;
            }
            else
            {
                statement_.inds_[position_][i] = i_ok;
            }
        }
    }
    else // gotData == false
    {
        // nothing to do here, vectors are truncated anyway
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
            if (statement_.inds_[position_][i] == i_null && (ind == NULL))
            {
                throw soci_error("Null value fetched and no indicator defined.");
            }
            else if (ind != NULL)
            {
                ind[i] = statement_.inds_[position_][i];
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
    std::vector<void*>::iterator it
        = std::find(statement_.intos_.begin(), statement_.intos_.end(), this);
    if (it != statement_.intos_.end())
        statement_.intos_.erase(it);
}
