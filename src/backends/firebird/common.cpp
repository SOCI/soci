//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "common.h"
#include <soci-backend.h>
#include <ibase.h> // FireBird
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>

namespace soci
{

namespace details
{

namespace firebird
{

char * allocBuffer(XSQLVAR* var)
{
    std::size_t size;
    if ((var->sqltype & ~1) == SQL_VARYING)
    {
        size = var->sqllen + sizeof(short);
    }
    else
    {
        size = var->sqllen;
    }

    return new char[size];
}

void tmEncode(short type, std::tm * src, void * dst)
{
    switch (type & ~1)
    {
        // In Interbase v6 DATE represents a date-only data type,
        // in InterBase v5 DATE represents a date+time data type.
    case SQL_TIMESTAMP:
        isc_encode_timestamp(src, static_cast<ISC_TIMESTAMP*>(dst));
        break;
    case SQL_TYPE_TIME:
        isc_encode_sql_time(src, static_cast<ISC_TIME*>(dst));
        break;
    case SQL_TYPE_DATE:
        isc_encode_sql_date(src, static_cast<ISC_DATE*>(dst));
        break;
    default:
        std::ostringstream msg;
        msg << "Unexpected type of date/time field (" << type << ")";
        throw soci_error(msg.str());
    }
}

void tmDecode(short type, void * src, std::tm * dst)
{
    switch (type & ~1)
    {
    case SQL_TIMESTAMP:
        isc_decode_timestamp(static_cast<ISC_TIMESTAMP*>(src), dst);
        break;
    case SQL_TYPE_TIME:
        isc_decode_sql_time(static_cast<ISC_TIME*>(src), dst);
        break;
    case SQL_TYPE_DATE:
        isc_decode_sql_date(static_cast<ISC_DATE*>(src), dst);
        break;
    default:
        std::ostringstream msg;
        msg << "Unexpected type of date/time field (" << type << ")";
        throw soci_error(msg.str());
    }
}

void setTextParam(char const * s, std::size_t size, char * buf_,
    XSQLVAR * var)
{
    short sz = 0;
    if (size < static_cast<std::size_t>(var->sqllen))
    {
        sz = static_cast<short>(size);
    }
    else
    {
        sz = var->sqllen;
    }

    if ((var->sqltype & ~1) == SQL_VARYING)
    {
        memcpy(buf_, &sz, sizeof(short));
        memcpy(buf_ + sizeof(short), s, sz);
    }
    else if ((var->sqltype & ~1) == SQL_TEXT)
    {
        memcpy(buf_, s, sz);
        if (sz < var->sqllen)
        {
            memset(buf_+sz, ' ', var->sqllen - sz);
        }
    }
    else
    {
        throw soci_error("Unexpected string type.");
    }
}

std::string getTextParam(XSQLVAR const *var)
{
    short size;
    std::size_t offset = 0;

    if ((var->sqltype & ~1) == SQL_VARYING)
    {
        size = *reinterpret_cast<short*>(var->sqldata);
        offset = sizeof(short);
    }
    else if ((var->sqltype & ~1) == SQL_TEXT)
    {
        size = var->sqllen;
    }
    else throw soci_error("Unexpected string type");

    return std::string(var->sqldata + offset, size);
}

} // namespace firebird

} // namespace details

} // namespace soci
