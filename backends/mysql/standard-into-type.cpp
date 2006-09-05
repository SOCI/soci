//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-mysql.h"
#include "common.h"

#include <ciso646>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::MySQL;


void MySQLStandardIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void MySQLStandardIntoTypeBackEnd::preFetch()
{
    // nothing to do here
}

void MySQLStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator *ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }
    
    if (gotData)
    {
        int pos = position_ - 1;
        mysql_data_seek(statement_.result_, statement_.currentRow_);
        MYSQL_ROW row = mysql_fetch_row(statement_.result_);
        if (row[pos] == NULL)
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
        const char *buf = row[pos] != NULL ? row[pos] : "";
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

void MySQLStandardIntoTypeBackEnd::cleanUp()
{
    // nothing to do here
}
