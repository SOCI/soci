// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#define SOCI_ODBC_SOURCE
#include "soci/soci-platform.h"
#include "soci/odbc/soci-odbc.h"
#include "soci/soci-unicode.h"
#include "soci-compiler.h"
#include "soci-exchange-cast.h"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

using namespace soci;
using namespace soci::details;

void* odbc_standard_use_type_backend::prepare_for_bind(
    SQLLEN &size, SQLSMALLINT &sqlType, SQLSMALLINT &cType)
{
    switch (type_)
    {
    // simple cases
    case x_int8:
        sqlType = supports_negative_tinyint() ? SQL_TINYINT : SQL_SMALLINT;
        cType = SQL_C_STINYINT;
        size = sizeof(int8_t);
        break;
    case x_uint8:
        sqlType = can_convert_to_unsigned_sql_type() ? SQL_TINYINT : SQL_SMALLINT;
        cType = SQL_C_UTINYINT;
        size = sizeof(uint8_t);
        break;
    case x_int16:
        sqlType = SQL_SMALLINT;
        cType = SQL_C_SSHORT;
        size = sizeof(int16_t);
        break;
    case x_uint16:
        sqlType = can_convert_to_unsigned_sql_type() ? SQL_SMALLINT : SQL_INTEGER;
        cType = SQL_C_USHORT;
        size = sizeof(uint16_t);
        break;
    case x_int32:
        sqlType = SQL_INTEGER;
        cType = SQL_C_SLONG;
        size = sizeof(int32_t);
        break;
    case x_uint32:
        sqlType = can_convert_to_unsigned_sql_type() ? SQL_INTEGER : SQL_BIGINT;
        cType = SQL_C_ULONG;
        size = sizeof(uint32_t);
        break;
    case x_int64:
        if (use_string_for_bigint())
        {
          sqlType = SQL_NUMERIC;
          cType = SQL_C_CHAR;
          size = max_bigint_length;
          buf_ = new char[size];
          snprintf(buf_, size, "%" LL_FMT_FLAGS "d",
                   static_cast<long long>(exchange_type_cast<x_int64>(data_)));
          indHolder_ = SQL_NTS;
        }
        else // Normal case, use ODBC support.
        {
          sqlType = SQL_BIGINT;
          cType = SQL_C_SBIGINT;
          size = sizeof(int64_t);
        }
        break;
    case x_uint64:
        if (use_string_for_bigint() || !can_convert_to_unsigned_sql_type())
        {
          sqlType = SQL_NUMERIC;
          cType = SQL_C_CHAR;
          size = max_bigint_length;
          buf_ = new char[size];
          snprintf(buf_, size, "%" LL_FMT_FLAGS "u",
                   static_cast<unsigned long long>(exchange_type_cast<x_uint64>(data_)));
          indHolder_ = SQL_NTS;
        }
        else
        {
          sqlType = SQL_BIGINT;
          cType = SQL_C_UBIGINT;
          size = sizeof(uint64_t);
        }
        break;
    case x_double:
        sqlType = SQL_DOUBLE;
        cType = SQL_C_DOUBLE;
        size = sizeof(double);
        break;

    case x_char:
        sqlType = SQL_CHAR;
        cType = SQL_C_CHAR;
        size = 2;
        buf_ = new char[size];
        buf_[0] = exchange_type_cast<x_char>(data_);
        buf_[1] = '\0';
        indHolder_ = SQL_NTS;
        break;
    case x_stdstring:
    {
        std::string const& s = exchange_type_cast<x_stdstring>(data_);

        copy_from_string(s, size, sqlType, cType);
    }
    break;
    case x_stdwstring:
    {
        std::wstring const& s = exchange_type_cast<x_stdwstring>(data_);

        copy_from_string(s, size, sqlType, cType);
    }
    break;
    case x_stdtm:
    {
        std::tm const& t = exchange_type_cast<x_stdtm>(data_);

        sqlType = SQL_TIMESTAMP;
        cType = SQL_C_TIMESTAMP;
        buf_ = new char[sizeof(TIMESTAMP_STRUCT)];
        size = 19; // This number is not the size in bytes, but the number
                   // of characters in the date if it was written out
                   // yyyy-mm-dd hh:mm:ss

        // See comment for the use of this macro in standard-into-type.cpp.
        SOCI_GCC_WARNING_SUPPRESS(cast-align)

        TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf_);

        SOCI_GCC_WARNING_RESTORE(cast-align)

        ts->year = static_cast<SQLSMALLINT>(t.tm_year + 1900);
        ts->month = static_cast<SQLUSMALLINT>(t.tm_mon + 1);
        ts->day = static_cast<SQLUSMALLINT>(t.tm_mday);
        ts->hour = static_cast<SQLUSMALLINT>(t.tm_hour);
        ts->minute = static_cast<SQLUSMALLINT>(t.tm_min);
        ts->second = static_cast<SQLUSMALLINT>(t.tm_sec);
        ts->fraction = 0;
    }
    break;

    case x_longstring:
        copy_from_string(exchange_type_cast<x_longstring>(data_).value,
                         size, sqlType, cType);
        break;
    case x_xmltype:
        copy_from_string(exchange_type_cast<x_xmltype>(data_).value,
                         size, sqlType, cType);
        break;

    // unsupported types
    default:
        throw soci_error("Use element used with non-supported type.");
    }

    // Return either the pointer to C++ data itself or the buffer that we
    // allocated, if any.
    return buf_ ? buf_ : data_;
}

void odbc_standard_use_type_backend::copy_from_string(
        std::string const& s,
        SQLLEN& size,
        SQLSMALLINT& sqlType,
        SQLSMALLINT& cType
    )
{
    size = s.size();
    sqlType = size >= ODBC_MAX_COL_SIZE ? SQL_LONGVARCHAR : SQL_VARCHAR;
    cType = SQL_C_CHAR;
    buf_ = new char[size+1];
    memcpy(buf_, s.c_str(), size);
    buf_[size++] = '\0';
    indHolder_ = SQL_NTS;
}

void odbc_standard_use_type_backend::copy_from_string(
        const std::wstring& s,
        SQLLEN& size,
        SQLSMALLINT& sqlType,
        SQLSMALLINT& cType
    )
{
    auto const len = wide_to_utf16(s, nullptr, 0);

    size = static_cast<SQLLEN>((len + 1) * sizeof(SQLWCHAR));
    sqlType = size >= ODBC_MAX_COL_SIZE ? SQL_WLONGVARCHAR : SQL_WVARCHAR;
    cType = SQL_C_WCHAR;
    buf_ = new char[size];

    char16_t* const wbuf = reinterpret_cast<char16_t*>(buf_);
    wide_to_utf16(s, wbuf, len);
    wbuf[len] = u'\0';

    indHolder_ = SQL_NTS;
}

void odbc_standard_use_type_backend::bind_by_pos(
    int &position, void *data, exchange_type type, bool /* readOnly */)
{
    if (statement_.boundByName_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    position_ = position++;
    data_ = data;
    type_ = type;

    statement_.boundByPos_ = true;
}

void odbc_standard_use_type_backend::bind_by_name(
    std::string const &name, void *data, exchange_type type, bool /* readOnly */)
{
    if (statement_.boundByPos_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    int position = -1;
    int count = 1;

    for (auto const& s : statement_.names_)
    {
        if (s == name)
        {
            position = count;
            break;
        }
        count++;
    }

    if (position == -1)
    {
        std::ostringstream ss;
        ss << "Unable to find name '" << name << "' to bind to";
        throw soci_error(ss.str());
    }

    position_ = position;
    data_ = data;
    type_ = type;

    statement_.boundByName_ = true;
}

void odbc_standard_use_type_backend::pre_use(indicator const *ind)
{
    // first deal with data
    SQLSMALLINT sqlType(0);
    SQLSMALLINT cType(0);
    SQLLEN size(0);
    SQLLEN bufLen(0);

    void* const sqlData = prepare_for_bind(size, sqlType, cType);

    // If the indicator is i_null, we need to pass the corresponding value to
    // the ODBC function, and we have to do it without changing indHolder_
    // itself because we may need to use its original value again when we're
    // called the next when executing a prepared statement multiple times.
    //
    // So use separate holder variables depending on whether we need to insert
    // null or not.
    static const SQLLEN indHolderNull = SQL_NULL_DATA;

    SQLRETURN rc = SQLBindParameter(statement_.hstmt_,
                                    static_cast<SQLUSMALLINT>(position_),
                                    SQL_PARAM_INPUT,
                                    cType, sqlType, size, 0,
                                    sqlData, bufLen,
                                    ind && *ind == i_null
                                        ? const_cast<SQLLEN *>(&indHolderNull)
                                        : &indHolder_);

    if (is_odbc_error(rc))
    {
        std::ostringstream ss;
        ss << "binding input parameter #" << position_;
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, ss.str());
    }
}

void odbc_standard_use_type_backend::post_use(bool gotData, indicator *ind)
{
    if (ind != NULL)
    {
        if (gotData)
        {
            if (indHolder_ == 0)
            {
                *ind = i_ok;
            }
            else if (indHolder_ == SQL_NULL_DATA)
            {
                *ind = i_null;
            }
            else
            {
                *ind = i_truncated;
            }
        }
    }

    clean_up();
}

void odbc_standard_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
