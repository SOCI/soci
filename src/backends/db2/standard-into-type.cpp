//
// Copyright (C) 2011-2013 Denis Chapligin
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_DB2_SOURCE
#include "soci/db2/soci-db2.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include "common.h"
#include <cstdint>
#include <ctime>

using namespace soci;
using namespace soci::details;


void db2_standard_into_type_backend::define_by_pos(
    int & position, void * data, exchange_type type)
{
    this->data = data;
    this->type = type;
    this->position = position;
    position++;

    SQLUINTEGER size = 0;

    switch (type)
    {
    case x_char:
        cType = SQL_C_CHAR;
        size = sizeof(char) + 1;
        buf = new char[size];
        data = buf;
        break;
    case x_stdstring:
        cType = SQL_C_CHAR;
        // Patch: set to min between column size and 100MB (used ot be 32769)
        // Column size for text data type can be too large for buffer allocation
        size = static_cast<SQLUINTEGER>(statement_.column_size(this->position));
        size = size > details::db2::cli_max_buffer ? details::db2::cli_max_buffer : size;
        size++;
        buf = new char[size];
        data = buf;
        break;
    case x_int8:
        cType = SQL_C_STINYINT;
        size = sizeof(int8_t);
        break;
    case x_uint8:
        cType = SQL_C_UTINYINT;
        size = sizeof(uint8_t);
        break;
    case x_int16:
        cType = SQL_C_SSHORT;
        size = sizeof(int16_t);
        break;
    case x_uint16:
        cType = SQL_C_USHORT;
        size = sizeof(uint16_t);
        break;
    case x_int32:
        cType = SQL_C_SLONG;
        size = sizeof(SQLINTEGER);
        break;
    case x_uint32:
        cType = SQL_C_ULONG;
        size = sizeof(SQLUINTEGER);
        break;
    case x_int64:
        cType = SQL_C_SBIGINT;
        size = sizeof(int64_t);
        break;
    case x_uint64:
        cType = SQL_C_UBIGINT;
        size = sizeof(uint64_t);
        break;
    case x_double:
        cType = SQL_C_DOUBLE;
        size = sizeof(double);
        break;
    case x_stdtm:
        cType = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        buf = new char[size];
        data = buf;
        break;
    case x_rowid:
        cType = SQL_C_UBIGINT;
        size = sizeof(int64_t);
        break;
    default:
        throw soci_error("Into element used with non-supported type.");
    }

    valueLen = 0;

    SQLRETURN cliRC = SQLBindCol(statement_.hStmt, static_cast<SQLUSMALLINT>(this->position),
        static_cast<SQLUSMALLINT>(cType), data, size, &valueLen);
    if (cliRC != SQL_SUCCESS)
    {
        throw db2_soci_error("Error while pre-fething into type",cliRC);
    }
}

void db2_standard_into_type_backend::pre_fetch()
{
    //...
}

void db2_standard_into_type_backend::post_fetch(
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
        if (SQL_NULL_DATA == valueLen)
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
        if (type == x_char)
        {
            exchange_type_cast<x_char>(data) = buf[0];
        }
        if (type == x_stdstring)
        {
            std::string& s = exchange_type_cast<x_stdstring>(data);
            s = buf;
            if (s.size() >= (details::db2::cli_max_buffer - 1))
            {
                throw soci_error("Buffer size overflow; maybe got too large string");
            }
        }
        else if (type == x_stdtm)
        {
            std::tm& t = exchange_type_cast<x_stdtm>(data);

            TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(buf);

            details::mktime_from_ymdhms(t,
                                        ts->year, ts->month, ts->day,
                                        ts->hour, ts->minute, ts->second);
        }
    }
}

void db2_standard_into_type_backend::clean_up()
{
    if (buf)
    {
        delete [] buf;
        buf = 0;
    }
}
