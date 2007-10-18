//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <ctime>

using namespace soci;
using namespace soci::details;


void odbc_standard_into_type_backend::define_by_pos(
    int & position, void * data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;

    SQLINTEGER size = 0;

    switch (type_)
    {
    case eXChar:
        odbcType_ = SQL_C_CHAR;
        size = sizeof(char) + 1;
        buf_ = new char[size];
        data = buf_;
        break;
    case eXCString:
    {
        details::cstring_descriptor *desc = static_cast<cstring_descriptor *>(data);
        odbcType_ = SQL_C_CHAR;
        data = desc->str_;
        size = static_cast<SQLINTEGER>(desc->bufSize_);
    }
    break;
    case eXStdString:
        odbcType_ = SQL_C_CHAR;
        size = 4000;
        buf_ = new char[size]+1;
        data = buf_;
        break;
    case eXShort:
        odbcType_ = SQL_C_SSHORT;
        size = sizeof(short);
        break;
    case eXInteger:
        odbcType_ = SQL_C_SLONG;
        size = sizeof(long);
        break;
    case eXUnsignedLong:
        odbcType_ = SQL_C_ULONG;
        size = sizeof(unsigned long);
        break;
    case eXDouble:
        odbcType_ = SQL_C_DOUBLE;
        size = sizeof(double);
        break;
    case eXStdTm:
        odbcType_ = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        buf_ = new char[size];
        data = buf_;
        break;
    case eXRowID:
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
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                            "into type pre_fetch");
    }
}

void odbc_standard_into_type_backend::pre_fetch()
{
    //...
}

void odbc_standard_into_type_backend::post_fetch(
    bool gotData, bool calledFromFetch, eIndicator * ind)
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

            *ind = eNull;
            return;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = eOK;
            }
        }

        // only std::string and std::tm need special handling
        if (type_ == eXChar)
        {
            char *c = static_cast<char*>(data_);
            *c = buf_[0];
        }
        if (type_ == eXCString)
        {
            if (ind != NULL)
            {
                details::cstring_descriptor *desc = static_cast<cstring_descriptor *>(data_);
                int size = static_cast<SQLINTEGER>(desc->bufSize_);
                if (size < valueLen_)
                    *ind = eTruncated;
            }
        }
        if (type_ == eXStdString)
        {
            std::string *s = static_cast<std::string *>(data_);
            *s = buf_;
        }
        else if (type_ == eXStdTm)
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
    }
    else // no data retrieved
    {
        if (ind != NULL)
        {
            *ind = eNoData;
        }
        else
        {
            throw soci_error("No data fetched and no indicator defined.");
        }
    }
}

void odbc_standard_into_type_backend::clean_up()
{
    if (!buf_)
    {
        delete [] buf_;
        buf_ = 0;
    }
}
