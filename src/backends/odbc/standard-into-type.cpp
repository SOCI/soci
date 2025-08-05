//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci/soci-platform.h"
#include "soci/soci-unicode.h"
#include "soci/odbc/soci-odbc.h"
#include "soci-compiler.h"
#include "soci-cstrtoi.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include <cstdint>
#include <ctime>

using namespace soci;
using namespace soci::details;

void odbc_standard_into_type_backend::define_by_pos(
    int & position, void * data, exchange_type type)
{
    data_ = data;
    type_ = type;
    position_ = position++;

    SQLUINTEGER size = 0;
    switch (type_)
    {
    case x_char:
        odbcType_ = SQL_C_CHAR;
        size = sizeof(char) + 1;
        buf_ = new char[size];
        data = buf_;
        break;
    case x_stdstring:
    case x_longstring:
    case x_xmltype:
        odbcType_ = SQL_C_CHAR;
        // For LONGVARCHAR fields the returned size is ODBC_MAX_COL_SIZE
        // (or 0 for some backends), but this doesn't correspond to the actual
        // field size, which can be (much) greater. For now we just used
        // a buffer of huge (100MiB) hardcoded size, which is clearly not
        // ideal, but changing this would require using SQLGetData() and is
        // not trivial, so for now we're stuck with this suboptimal solution.
        size = static_cast<SQLUINTEGER>(statement_.column_size(position_));
        size = (size >= ODBC_MAX_COL_SIZE || size == 0) ? odbc_max_buffer_length : size;
        size++;
        buf_ = new char[size];
        data = buf_;
        break;
    case x_stdwstring:
        odbcType_ = SQL_C_WCHAR;
        // Do exactly the same thing here as for x_stdstring above.
        size = static_cast<SQLUINTEGER>(statement_.column_size(position_));
        size = (size >= ODBC_MAX_COL_SIZE || size == 0) ? odbc_max_buffer_length : size;
        size++;
        size = size * sizeof(SQLWCHAR); // size in bytes
        buf_ = new char[size];
        data = buf_;
        break;
    case x_int8:
        odbcType_ = SQL_C_STINYINT;
        size = sizeof(int8_t);
        break;
    case x_uint8:
        odbcType_ = SQL_C_UTINYINT;
        size = sizeof(uint8_t);
        break;
    case x_int16:
        odbcType_ = SQL_C_SSHORT;
        size = sizeof(int16_t);
        break;
    case x_uint16:
        odbcType_ = SQL_C_USHORT;
        size = sizeof(uint16_t);
        break;
    case x_int32:
        odbcType_ = SQL_C_SLONG;
        size = sizeof(int32_t);
        break;
    case x_uint32:
        odbcType_ = SQL_C_ULONG;
        size = sizeof(uint32_t);
        break;
    case x_int64:
        if (use_string_for_bigint())
        {
          odbcType_ = SQL_C_CHAR;
          size = max_bigint_length;
          buf_ = new char[size];
          data = buf_;
        }
        else // Normal case, use ODBC support.
        {
          odbcType_ = SQL_C_SBIGINT;
          size = sizeof(int64_t);
        }
        break;
    case x_uint64:
        if (use_string_for_bigint())
        {
          odbcType_ = SQL_C_CHAR;
          size = max_bigint_length;
          buf_ = new char[size];
          data = buf_;
        }
        else // Normal case, use ODBC support.
        {
          odbcType_ = SQL_C_UBIGINT;
          size = sizeof(uint64_t);
        }
        break;
    case x_double:
        odbcType_ = SQL_C_DOUBLE;
        size = sizeof(double);
        break;
    case x_stdtm:
        odbcType_ = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        buf_ = new char[size];
        data = buf_;
        break;
    case x_rowid:
        odbcType_ = SQL_C_ULONG;
        size = sizeof(unsigned long);
        break;
    default:
        throw soci_error("Into element used with non-supported type.");
    }

    valueLen_ = 0;

    SQLRETURN rc = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_),
        static_cast<SQLUSMALLINT>(odbcType_), data, size, &valueLen_);
    if (is_odbc_error(rc))
    {
        std::ostringstream ss;
        ss << "binding output column #" << position_;
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, ss.str());
    }
}

void odbc_standard_into_type_backend::pre_fetch()
{
    //...
}

void odbc_standard_into_type_backend::post_fetch(
    bool gotData, bool calledFromFetch, indicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    if (gotData)
    {
        // first, deal with indicators
        if (SQL_NULL_DATA == get_sqllen_from_value(valueLen_))
        {
            if (ind == NULL)
            {
                throw soci_error(
                    "Null value fetched and no indicator defined.");
            }

            *ind = i_null;
            return;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = i_ok;
            }
        }

        // only std::string and std::tm need special handling
        if (type_ == x_char)
        {
            exchange_type_cast<x_char>(data_) = buf_[0];
        }
        else if (type_ == x_stdstring)
        {
            std::string& s = exchange_type_cast<x_stdstring>(data_);
            s = buf_;
            if (s.size() >= (odbc_max_buffer_length - 1))
            {
                throw soci_error("Buffer size overflow; maybe got too large string");
            }
        }
        else if (type_ == x_stdwstring)
        {
            std::wstring& s = exchange_type_cast<x_stdwstring>(data_);
            s = utf16_to_wide(reinterpret_cast<char16_t*>(buf_));
            if (s.size() * sizeof(wchar_t) >= (odbc_max_buffer_length - 1))
            {
                throw soci_error("Buffer size overflow; maybe got too large string");
            }
        }
        else if (type_ == x_longstring)
        {
            exchange_type_cast<x_longstring>(data_).value = buf_;
        }
        else if (type_ == x_xmltype)
        {
            exchange_type_cast<x_xmltype>(data_).value = buf_;
        }
        else if (type_ == x_stdtm)
        {
            std::tm& t = exchange_type_cast<x_stdtm>(data_);

            // Our pointer should, in fact, be sufficiently aligned, as we
            // allocate it on the heap, but gcc on ARMv7hl warns about this not
            // being the case, so just suppress this warning explicitly here.
            SOCI_GCC_WARNING_SUPPRESS(cast-align)

            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf_);

            SOCI_GCC_WARNING_RESTORE(cast-align)

            details::mktime_from_ymdhms(t,
                                        ts->year, ts->month, ts->day,
                                        ts->hour, ts->minute, ts->second);
        }
        else if (type_ == x_int64 && use_string_for_bigint())
        {
          int64_t ll = exchange_type_cast<x_int64>(data_);
          if (!cstring_to_integer(ll, buf_))
          {
            throw soci_error("Failed to parse the returned 64-bit integer value");
          }
        }
        else if (type_ == x_uint64 && use_string_for_bigint())
        {
          uint64_t ll = exchange_type_cast<x_uint64>(data_);
          if (!cstring_to_unsigned(ll, buf_))
          {
            throw soci_error("Failed to parse the returned 64-bit integer value");
          }
        }
    }
}

void odbc_standard_into_type_backend::clean_up()
{
    if (buf_)
    {
        delete [] buf_;
        buf_ = 0;
    }
}
