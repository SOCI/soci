//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include <soci-platform.h>
#include "soci-odbc.h"
#include <ctime>
#include <stdio.h>  // sscanf()

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
	case x_stdwstring:
		{
			odbcType_ = SQL_C_WCHAR;
			//statement already knows max size for NTEXT columns !
			size = statement_.column_size(position_) * sizeof(wchar_t);
			//superset size if the string is presized longer than max definition
			std::wstring *s = static_cast<std::wstring *>(data_);
			if (s->length() > size) size = s->length();
			size += sizeof(wchar_t); //add for term 0
			buf_ = new char[size];
			data = buf_;
		}
		break;
    case x_stdstring:
		{
			odbcType_ = SQL_C_CHAR;
			//statement already knows max size for TEXT columns!
			size = statement_.column_size(position_);
			//superset size if the string is presized longer than max definition
			std::string *s = static_cast<std::string *>(data_);
			if (s->length() > size) size = s->length();
			size++;
			buf_ = new char[size];
			data = buf_;
		}
        break;
    case x_short:
        odbcType_ = SQL_C_SSHORT;
        size = sizeof(short);
        break;
    case x_integer:
        odbcType_ = SQL_C_SLONG;
        size = sizeof(int);
        break;
    case x_long_long:
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
          size = sizeof(long long);
        }
        break;
    case x_unsigned_long_long:
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
          size = sizeof(unsigned long long);
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
	case x_binary:
		{
			odbcType_ = SQL_C_BINARY;
			//statement already knows max size for CLOB, BINARY columns!
			size = statement_.column_size(position_);
			//superset size if the blob is presized longer than max definition
			soci::binarydata *s = static_cast<soci::binarydata *>(data_);
			if (s->size() > size) size = s->size();
			size++;
			buf_ = new char[size];
			data = buf_;
		}
        break;
    default:
        throw soci_error("Into element used with non-supported type.");
    }

    valueLen_ = 0;

    SQLRETURN rc = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_),
        static_cast<SQLUSMALLINT>(odbcType_), data, size, &valueLen_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                            "into type pre_fetch");
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
        if (SQL_NULL_DATA == valueLen_)
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
            char *c = static_cast<char*>(data_);
            *c = buf_[0];
        }
		if (type_ == x_binary)
		{
			soci::binarydata *s = static_cast<soci::binarydata *>(data_);
			s->assign(buf_,  buf_ + valueLen_);
		}
        else if (type_ == x_stdwstring)
        {
            std::wstring *s = static_cast<std::wstring *>(data_);
			//collect max for overflow check
			std::size_t max = statement_.session_.get_max_text_length();
			//respect supersets
			if (s->size() > max) max = s->size();
			//assign buffer
            *s = (wchar_t*)buf_;
			//check possible overflow/truncation now
			if (s->size() >= max)
            {
                throw soci_error("Buffer size overflow; maybe got too large string");
            }
        }
        else if (type_ == x_stdstring)
        {
            std::string *s = static_cast<std::string *>(data_);
			//collect max for overflow check
			std::size_t max = statement_.session_.get_max_text_length();
			//respect supersets
			if (s->size() > max) max = s->size();
			//assign buffer
            *s = buf_;
			//check possible overflow/truncation now
			if (s->size() >= max)
            {
                throw soci_error("Buffer size overflow; maybe got too large string");
            }
        }
        else if (type_ == x_stdtm)
        {
            std::tm *t = static_cast<std::tm *>(data_);

            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf_);
            t->tm_isdst = -1;
            t->tm_year = ts->year - 1900;
            t->tm_mon = ts->month - 1;
            t->tm_mday = ts->day;
            t->tm_hour = ts->hour;
            t->tm_min = ts->minute;
            t->tm_sec = ts->second;

            // normalize and compute the remaining fields
            std::mktime(t);
        }
        else if (type_ == x_long_long && use_string_for_bigint())
        {
          long long *ll = static_cast<long long *>(data_);
          if (sscanf(buf_, "%" LL_FMT_FLAGS "d", ll) != 1)
          {
            throw soci_error("Failed to parse the returned 64-bit integer value");
          }
        }
        else if (type_ == x_unsigned_long_long && use_string_for_bigint())
        {
          unsigned long long *ll = static_cast<unsigned long long *>(data_);
          if (sscanf(buf_, "%" LL_FMT_FLAGS "u", ll) != 1)
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
