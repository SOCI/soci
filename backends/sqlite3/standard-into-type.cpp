//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#include "common.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Sqlite3;

void Sqlite3StandardIntoTypeBackEnd::defineByPos(int & position, void * data
                                                 , eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;    
}

void Sqlite3StandardIntoTypeBackEnd::preFetch()
{
    // ...
}

void Sqlite3StandardIntoTypeBackEnd::postFetch(bool gotData, 
                                               bool calledFromFetch, 
                                               eIndicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    // sqlite columns start at 0
    int pos = position_ - 1;

    if (gotData)
    {
        // first, deal with indicators
        if (sqlite3_column_type(statement_.stmt_, pos) == SQLITE_NULL)
        {
            if (ind == NULL)
            {
                throw SOCIError(
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

        const char *buf = 
        reinterpret_cast<const char*>(sqlite3_column_text(
                                          statement_.stmt_, 
                                          pos));

        if (!buf)
            buf = "";


        switch (type_)
        {
        case eXChar:
        {
            char *dest = static_cast<char*>(data_);
            *dest = *buf;
        }
        break;
        case eXCString:
        {
            CStringDescriptor *strDescr
            = static_cast<CStringDescriptor *>(data_);

            std::strncpy(strDescr->str_, buf, strDescr->bufSize_ - 1);
            strDescr->str_[strDescr->bufSize_ - 1] = '\0';

            if (std::strlen(buf) >= strDescr->bufSize_ && ind != NULL)
            {
                *ind = eTruncated;
            }
        }
        break;
        case eXStdString:
        {
            std::string *dest = static_cast<std::string *>(data_);
            dest->assign(buf);
        }
        break;
        case eXShort:
        {
            short *dest = static_cast<short*>(data_);
            long val = strtol(buf, NULL, 10);
            *dest = static_cast<short>(val);
        }
        break;
        case eXInteger:
        {
            int *dest = static_cast<int*>(data_);
            long val = strtol(buf, NULL, 10);
            *dest = static_cast<int>(val);
        }
        break;
        case eXUnsignedLong:
        {
            unsigned long *dest = static_cast<unsigned long *>(data_);
            long long val = strtoll(buf, NULL, 10);
            *dest = static_cast<unsigned long>(val);
        }
        break;
        case eXDouble:
        {
            double *dest = static_cast<double*>(data_);
            double val = strtod(buf, NULL);
            *dest = static_cast<double>(val);
        }
        break;
        case eXStdTm:
        {
            // attempt to parse the string and convert to std::tm
            std::tm *dest = static_cast<std::tm *>(data_);
            parseStdTm(buf, *dest);
        }
        break;
        case eXRowID:
        {
            // RowID is internally identical to unsigned long

            RowID *rid = static_cast<RowID *>(data_);
            Sqlite3RowIDBackEnd *rbe
            = static_cast<Sqlite3RowIDBackEnd *>(
                rid->getBackEnd());
            long long val = strtoll(buf, NULL, 10);
            rbe->value_ = static_cast<unsigned long>(val);
        }
        break;
        case eXBLOB:
        {
#ifdef SQLITE_ENABLE_COLUMN_METADATA

            BLOB *b = static_cast<BLOB *>(data_);
            Sqlite3BLOBBackEnd *bbe =
                static_cast<Sqlite3BLOBBackEnd *>(b->getBackEnd());
            
            buf = reinterpret_cast<const char*>(sqlite3_column_blob(
                                          statement_.stmt_, 
                                          pos));

            int len = sqlite3_column_bytes(statement_.stmt_, pos);
            const char *tableName =
                sqlite3_column_table_name(statement_.stmt_, 
                                          pos);
            const char *columnName = sqlite3_column_name(statement_.stmt_, 
                                          pos);
            bbe->setData(tableName, columnName, buf, len);
#endif
        }
        break;

        default:
            throw SOCIError("Into element used with non-supported type.");
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
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void Sqlite3StandardIntoTypeBackEnd::cleanUp()
{
    // ...
}
