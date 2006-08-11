// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "soci.h"
#include "soci-odbc.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>

using namespace SOCI;
using namespace SOCI::details;

void ODBCStandardUseTypeBackEnd::prepareForBind(
    void *&data, SQLUINTEGER &size, SQLSMALLINT &sqlType, SQLSMALLINT &cType)
{
    switch (type_)
    {
    // simple cases
    case eXShort:
        sqlType = SQL_SMALLINT;
        cType = SQL_C_SSHORT;
        size = sizeof(short);
        break;
    case eXInteger:
        sqlType = SQL_INTEGER;
        cType = SQL_C_SLONG;
        size = sizeof(int);
        break;
    case eXUnsignedLong:
        sqlType = SQL_BIGINT;
        cType = SQL_C_ULONG;
        size = sizeof(unsigned long);
        break;
    case eXDouble:
        sqlType = SQL_DOUBLE;
        cType = SQL_C_DOUBLE;
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management
    case eXChar:
        sqlType = SQL_CHAR;
        cType = SQL_C_CHAR;
        size = sizeof(char)+1;
        buf_ = new char[size];
        data = buf_;
        indHolder_ = SQL_NTS;
        break;
    case eXCString:
    {
        details::CStringDescriptor *desc = static_cast<CStringDescriptor *>(data);
        sqlType = SQL_VARCHAR;
        cType = SQL_C_CHAR;
        data = desc->str_;
        size = static_cast<SQLUINTEGER>(desc->bufSize_);
        indHolder_ = SQL_NTS;
    }
    break;
    case eXStdString:
    {
        std::string *s = static_cast<std::string *>(data);
        sqlType = SQL_VARCHAR;
        cType = SQL_C_CHAR;
        size = 255; // !FIXME this is not sufficent
        buf_ = new char[size];
        data = buf_;
        indHolder_ = SQL_NTS;
    }
    break;
    case eXStdTm:
        sqlType = SQL_TYPE_TIMESTAMP;
        cType = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        buf_ = new char[size];
        data = buf_;
        indHolder_ = size;
        break;

    case eXBLOB:
    {
//         sqlType = SQL_VARBINARY;
//         cType = SQL_C_BINARY;
        
//         BLOB *b = static_cast<BLOB *>(data);
        
//         ODBCBLOBBackEnd *bbe
//         = static_cast<ODBCBLOBBackEnd *>(b->getBackEnd());
        
//         size = 0;
//         indHolder_ = size;
        //TODO            data = &bbe->lobp_;
    }
    break;
    case eXStatement:
    case eXRowID:
        break;
    }
}

void ODBCStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    SQLSMALLINT sqlType;
    SQLSMALLINT cType;
    SQLUINTEGER size;

    prepareForBind(data, size, sqlType, cType);

	SQLRETURN rc = SQLBindParameter(statement_.hstmt_, position++, SQL_PARAM_INPUT, 
                                    cType, sqlType, size, 0, data, 0, &indHolder_);

    if (is_odbc_error(rc))
    {
        throw new ODBCSOCIError(SQL_HANDLE_STMT, statement_.hstmt_, 
                                "Binding by Position");
    }
}

void ODBCStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    int position = -1;
    int count = 1;
    
    for (std::vector<std::string>::iterator it = statement_.names_.begin(); 
         it != statement_.names_.end(); ++it)
    {
        if (*it == name)
        {
            position = count;
            break;
        }
        count++;
    }

    if (position != -1)
        bindByPos(position, data, type);
    else
    {
        std::ostringstream ss;
        ss << "Unable to find name '" << name << "' to bind to";
        throw SOCIError(ss.str().c_str());
    }
}

void ODBCStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXChar)
    {
        char *c = static_cast<char*>(data_);
        buf_[0] = *c;
        buf_[1] = '\0';
    }
    else if (type_ == eXStdString)
    {
        std::string *s = static_cast<std::string *>(data_);

        std::size_t const bufSize = 4000;
        std::size_t const sSize = s->size();
        std::size_t const toCopy =
            sSize < bufSize -1 ? sSize + 1 : bufSize - 1;
        strncpy(buf_, s->c_str(), toCopy);
        buf_[toCopy] = '\0';
    }
    else if (type_ == eXStdTm)
    {
        std::tm *t = static_cast<std::tm *>(data_);
        TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf_);

        ts->year = t->tm_year;
        ts->month = t->tm_mon;
        ts->day = t->tm_mday;
        ts->hour = t->tm_hour;
        ts->minute = t->tm_min;
        ts->second = t->tm_sec;
        ts->fraction = 0;
    }

    // then handle indicators
    if (ind != NULL && *ind == eNull)
    {
        indHolder_ = SQL_NULL_DATA; // null
    }
}

void ODBCStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        if (type_ == eXChar)
        {
            char *c = static_cast<char*>(data_);
            *c = buf_[0];
        }
        else if (type_ == eXStdString)
        {
            std::string *s = static_cast<std::string *>(data_);

            *s = buf_;
        }
        else if (type_ == eXStdTm)
        {
            std::tm *t = static_cast<std::tm *>(data_);
            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf_);
            t->tm_isdst = -1;
            t->tm_year = ts->year;
            t->tm_mon = ts->month;
            t->tm_mday = ts->day;
            t->tm_hour = ts->hour;
            t->tm_min = ts->minute;
            t->tm_sec = ts->second;

            // normalize and compute the remaining fields
            std::mktime(t);
        }
    }

    if (ind != NULL)
    {
        if (gotData == false)
        {
            *ind = eNoData;
        }
        else
        {
            if (indHolder_ == 0)
            {
                *ind = eOK;
            }
            else if (indHolder_ == SQL_NULL_DATA)
            {
                *ind = eNull;
            }
            else
            {
                *ind = eTruncated;
            }
        }
    }
    else
    {
        if (indHolder_ == SQL_NULL_DATA)
        {
            // fetched null and no indicator - programming error!
            throw SOCIError("Null value fetched and no indicator defined.");
        }

        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void ODBCStandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
