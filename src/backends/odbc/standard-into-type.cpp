//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci-platform.h"
#include "soci/soci-unicode.h"
#include "soci/odbc/soci-odbc.h"
#include "soci-compiler.h"
#include "soci-cstrtoi.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>
#include <fmt/format.h>

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
        SOCI_FALLTHROUGH;
    case x_stdwstring:
        {
            if (type_ == x_stdwstring)
                odbcType_ = SQL_C_WCHAR;

            // For LONGVARCHAR fields the returned size is ODBC_MAX_COL_SIZE
            // (or 0 for some backends), but this doesn't correspond to the
            // actual field size, which can be (much) greater, so we can't
            // allocate a buffer for them and use SQLGetData() to retrieve
            // their values instead, see get_data().
            std::size_t const colSize = statement_.column_size(position_);
            if (is_odbc_long_column(colSize))
            {
                isLong_ = true;
                break;
            }

            size = static_cast<SQLUINTEGER>(colSize + 1);
            if (type_ == x_stdwstring)
                size *= sizeof(SQLWCHAR); // size in bytes
            buf_ = new char[size];
            data = buf_;
        }
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

    odbcData_ = data;
    odbcSize_ = size;

    if (isLong_)
        statement_.add_long_column(position_);

    if (statement_.is_get_data_column(position_))
    {
        statement_.getDataIntos_.push_back(this);

        // Don't bind it.
        return;
    }

    SQLRETURN rc = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_),
        static_cast<SQLUSMALLINT>(odbcType_), data, size, &valueLen_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_, fmt::format("binding output column #{}", position_));
    }
}

namespace
{

// Common implementation of odbc_standard_type_backend_base::get_long_data()
// for narrow and wide strings.
//
// If fix32BitLen is true, the driver uses 32-bit SQLLEN, see
// odbc_standard_type_backend_base::requires_noncompliant_32bit_sqllen().
template <typename CharT>
bool
do_get_long_data(SQLHSTMT hstmt,
                 SQLUSMALLINT colNum,
                 SQLSMALLINT cType,
                 bool fix32BitLen,
                 std::basic_string<CharT>& s)
{
    // Call SQLGetData() with the buffer of the given length, in characters,
    // including the space for the trailing NUL, and return the number of
    // characters that were available before this call in avail, unless it is
    // unknown or the value is NULL, in which case avail is set to
    // SQL_NO_TOTAL or SQL_NULL_DATA respectively.
    auto const get_data = [=](CharT* buf, std::size_t len, SQLLEN& avail)
    {
        SQLLEN ind = 0;
        SQLRETURN const rc = SQLGetData(hstmt, colNum, cType, buf,
                                        static_cast<SQLLEN>(len * sizeof(CharT)),
                                        &ind);
        if (is_odbc_error(rc))
        {
            throw odbc_soci_error(SQL_HANDLE_STMT, hstmt,
                                  fmt::format("getting data of column #{}", colNum));
        }

        if (fix32BitLen)
        {
            int ind32;
            std::memcpy(&ind32, &ind, sizeof(ind32));
            ind = ind32;
        }

        if (ind == SQL_NULL_DATA || ind == SQL_NO_TOTAL)
            avail = ind;
        else
            avail = ind / static_cast<SQLLEN>(sizeof(CharT));

        return rc;
    };

    // Return true if we got all the data available, given the result of
    // get_data() called with a buffer of the given length (including NUL):
    // note that we can't rely on the return code being SQL_SUCCESS for this,
    // as SQL_SUCCESS_WITH_INFO could be returned for some other reason, so
    // check the length too.
    auto const got_all = [](SQLRETURN rc, SQLLEN avail, std::size_t len)
    {
        return rc == SQL_SUCCESS ||
                (avail != SQL_NO_TOTAL && static_cast<std::size_t>(avail) < len);
    };

    // Use a fixed buffer for the first call: this avoids modifying the string
    // if the value turns out to be NULL and is sufficient for many values.
    CharT initialBuf[4096];
    std::size_t const initialLen = sizeof(initialBuf) / sizeof(CharT);

    SQLLEN avail;
    SQLRETURN rc = get_data(initialBuf, initialLen, avail);
    if (avail == SQL_NULL_DATA)
        return false;

    if (got_all(rc, avail, initialLen))
    {
        s.assign(initialBuf, static_cast<std::size_t>(avail));
        return true;
    }

    // The value was truncated and we got all but the last buffer character
    // (which was used for the trailing NUL).
    std::size_t got = initialLen - 1;
    s.assign(initialBuf, got);

    for (;;)
    {
        // Determine the size of the next chunk to retrieve: if we know how
        // much data remains, try to get all of it at once, but note that this
        // could be inaccurate (e.g. when the driver performs encoding
        // conversion), so we still loop until we get everything. And if we
        // don't know it, just double the total buffer size every time.
        std::size_t chunk = got;
        if (avail != SQL_NO_TOTAL &&
                static_cast<std::size_t>(avail) > got)
        {
            chunk = static_cast<std::size_t>(avail) - got;
        }

        s.resize(got + chunk + 1);

        SQLLEN const prevGot = static_cast<SQLLEN>(got);
        rc = get_data(&s[got], chunk + 1, avail);
        if (rc == SQL_NO_DATA)
            break;

        if (got_all(rc, avail, chunk + 1))
        {
            got += static_cast<std::size_t>(avail);
            break;
        }

        got += chunk;

        // Make avail the total size of the data, as after the first call.
        if (avail != SQL_NO_TOTAL)
            avail += prevGot;
    }

    s.resize(got);
    return true;
}

} // anonymous namespace

bool
odbc_standard_type_backend_base::get_long_data(SQLUSMALLINT colNum,
                                               std::string& s) const
{
    return do_get_long_data(statement_.hstmt_, colNum, SQL_C_CHAR,
                            requires_noncompliant_32bit_sqllen(), s);
}

bool
odbc_standard_type_backend_base::get_long_data(SQLUSMALLINT colNum,
                                               std::u16string& s) const
{
    static_assert(sizeof(SQLWCHAR) == sizeof(char16_t),
                  "SQLWCHAR must be UTF-16 for SQL_C_WCHAR to work");

    return do_get_long_data(statement_.hstmt_, colNum, SQL_C_WCHAR,
                            requires_noncompliant_32bit_sqllen(), s);
}

void odbc_standard_into_type_backend::get_data()
{
    SQLUSMALLINT const colNum = static_cast<SQLUSMALLINT>(position_);

    if (isLong_)
    {
        // Retrieve the value directly into the target string, if possible.
        bool gotValue;
        switch (type_)
        {
        case x_stdstring:
            gotValue = get_long_data(colNum, exchange_type_cast<x_stdstring>(data_));
            break;

        case x_longstring:
            gotValue = get_long_data(colNum, exchange_type_cast<x_longstring>(data_).value);
            break;

        case x_xmltype:
            gotValue = get_long_data(colNum, exchange_type_cast<x_xmltype>(data_).value);
            break;

        case x_stdwstring:
            {
                std::u16string s;
                gotValue = get_long_data(colNum, s);
                if (gotValue)
                    exchange_type_cast<x_stdwstring>(data_) = utf16_to_wide(s);
            }
            break;

        default:
            throw soci_error("Unexpected type of long column.");
        }

        // Value of valueLen_ doesn't matter, as long as it's not NULL, as the
        // value is already stored, so just use 0 for it in this case.
        valueLen_ = gotValue ? 0 : SQL_NULL_DATA;
        return;
    }

    valueLen_ = 0;
    SQLRETURN rc = SQLGetData(statement_.hstmt_, colNum, odbcType_,
                              odbcData_, odbcSize_, &valueLen_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                              fmt::format("getting data of column #{}", colNum));
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
            if (ind == nullptr)
            {
                throw soci_error(
                    "Null value fetched and no indicator defined.");
            }

            *ind = i_null;
            return;
        }
        else
        {
            if (ind != nullptr)
            {
                *ind = i_ok;
            }
        }

        // Values of long columns are retrieved directly into the target
        // variable by get_data(), so there is nothing more to do for them.
        if (isLong_)
            return;

        // only std::string and std::tm need special handling
        if (type_ == x_char)
        {
            exchange_type_cast<x_char>(data_) = buf_[0];
        }
        else if (type_ == x_stdstring)
        {
            std::string& s = exchange_type_cast<x_stdstring>(data_);
            s = buf_;
        }
        else if (type_ == x_stdwstring)
        {
            std::wstring& s = exchange_type_cast<x_stdwstring>(data_);
            s = utf16_to_wide(reinterpret_cast<char16_t*>(buf_));
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
        buf_ = nullptr;
    }

    auto& intos = statement_.getDataIntos_;
    auto const it = std::find(intos.begin(), intos.end(), this);
    if (it != intos.end())
        intos.erase(it);
}
